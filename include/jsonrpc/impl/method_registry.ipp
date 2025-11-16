#pragma once

#include <jsonrpc/detail/method_registry.hpp>
#include <jsonrpc/detail/method_wrapper.hpp>
#include <jsonrpc/types.hpp>
#include <jsonrpc/errors.hpp>

namespace jsonrpc {
namespace detail {

// ============================================================================
// 构造 & 并行配置
// ============================================================================

inline MethodRegistry::MethodRegistry()
    : batch_thread_count_(std::max<std::size_t>(2, std::thread::hardware_concurrency()))
    , batch_pool_(std::make_shared<boost::asio::thread_pool>(static_cast<unsigned>(batch_thread_count_)))
{
}

inline void MethodRegistry::set_batch_concurrency(std::size_t threads) {
    if (threads == 0) {
        threads = 1;
    }

    std::lock_guard<std::mutex> lock(pool_mutex_);
    batch_thread_count_ = threads;
    if (batch_pool_) {
        batch_pool_->stop();
        batch_pool_->join();
    }
    batch_pool_.reset(new boost::asio::thread_pool(static_cast<unsigned>(batch_thread_count_)));
}

inline std::shared_ptr<boost::asio::thread_pool> MethodRegistry::get_batch_pool() {
    std::lock_guard<std::mutex> lock(pool_mutex_);
    if (!batch_pool_) {
        batch_pool_ = std::make_shared<boost::asio::thread_pool>(static_cast<unsigned>(batch_thread_count_));
    }
    return batch_pool_;
}

// ============================================================================
// 注册方法
// ============================================================================

template<typename Func>
void MethodRegistry::register_method(const std::string& name, Func&& func) {
    auto wrapper = std::make_shared<MethodWrapperImpl<typename std::decay<Func>::type>>(
        std::forward<Func>(func)
    );

    std::lock_guard<std::mutex> lock(mutex_);
    methods_[name] = wrapper;
}

// ============================================================================
// 调用方法
// ============================================================================

inline Response MethodRegistry::invoke(const Request& request) {
    const std::string& method_name = request.method();
    const boost::json::value& params = request.params();
    const boost::json::value& id = request.id();

    try {
        // 查找方法
        std::shared_ptr<MethodWrapperBase> wrapper;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = methods_.find(method_name);
            if (it == methods_.end()) {
                throw Error(ErrorCode::MethodNotFound,
                    "方法不存在: " + method_name);
            }
            wrapper = it->second;
        }

        // 调用方法
        boost::json::value result = wrapper->invoke(params);

        // 构造成功响应
        return Response(result, id);

    } catch (const Error& e) {
        // JSON-RPC 错误，直接返回错误响应
        return Response(e, id);
    } catch (const std::exception& e) {
        // 其他异常，转换为 InternalError
        Error error(ErrorCode::InternalError,
            std::string("内部错误: ") + e.what());
        return Response(error, id);
    }
}

// ============================================================================
// 批量调用方法
// ============================================================================

inline std::vector<Response> MethodRegistry::invoke_batch(const std::vector<Request>& requests) {
    if (requests.empty()) {
        return {};
    }

    auto pool = get_batch_pool();
    struct IndexedResponse {
        std::size_t index;
        Response response;
    };

    std::mutex response_mutex;
    std::vector<IndexedResponse> indexed_responses;
    indexed_responses.reserve(requests.size());

    auto remaining = std::make_shared<std::atomic<std::size_t>>(requests.size());
    auto completion_promise = std::make_shared<std::promise<void>>();
    auto completion_future = completion_promise->get_future();

    for (std::size_t idx = 0; idx < requests.size(); ++idx) {
        Request request_copy = requests[idx];

        boost::asio::post(*pool, [this, idx, request_copy, &indexed_responses, &response_mutex, remaining, completion_promise]() mutable {
            Request request = std::move(request_copy);
            try {
                if (request.has_id()) {
                    Response resp = invoke(request);
                    std::lock_guard<std::mutex> lock(response_mutex);
                    indexed_responses.push_back(IndexedResponse{idx, std::move(resp)});
                } else {
                    invoke(request);
                }
            } catch (...) {
                if (request.has_id()) {
                    Response error_response(Error(ErrorCode::InternalError, "批量调用失败"), request.id());
                    std::lock_guard<std::mutex> lock(response_mutex);
                    indexed_responses.push_back(IndexedResponse{idx, std::move(error_response)});
                }
            }

            if (remaining->fetch_sub(1) == 1) {
                completion_promise->set_value();
            }
        });
    }

    completion_future.wait();

    std::sort(
        indexed_responses.begin(),
        indexed_responses.end(),
        [](const IndexedResponse& a, const IndexedResponse& b) {
            return a.index < b.index;
        }
    );

    std::vector<Response> responses;
    responses.reserve(indexed_responses.size());
    for (auto& entry : indexed_responses) {
        responses.push_back(std::move(entry.response));
    }

    return responses;
}

} // namespace detail
} // namespace jsonrpc
