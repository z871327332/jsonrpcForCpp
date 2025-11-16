#pragma once

#include <jsonrpc/client.hpp>
#include <jsonrpc/detail/client_session.hpp>
#include <jsonrpc/detail/protocol.hpp>
#include <jsonrpc/detail/type_converter.hpp>
#include <boost/asio.hpp>
#include <memory>
#include <atomic>

namespace jsonrpc {

// ============================================================================
// Client::Impl 类（Pimpl 实现）
// ============================================================================

class Client::Impl {
public:
    /**
     * @brief 构造 Impl
     * @param host 服务器地址
     * @param port 服务器端口
     */
    Impl(const std::string& host, unsigned short port)
        : io_context_()
        , host_(host)
        , port_(std::to_string(port))
        , timeout_(std::chrono::seconds(30))  // 默认 30 秒超时
        , next_id_(1)
    {
    }

    /**
     * @brief 获取 io_context
     */
    boost::asio::io_context& get_io_context() {
        return io_context_;
    }

    /**
     * @brief 设置超时时间
     */
    void set_timeout(std::chrono::milliseconds timeout) {
        timeout_ = timeout;
    }

    /**
     * @brief 创建会话
     */
    std::shared_ptr<detail::ClientSession> create_session() {
        return std::make_shared<detail::ClientSession>(
            io_context_,
            host_,
            port_,
            timeout_,
            logger_
        );
    }

    /**
     * @brief 生成唯一请求 ID
     */
    boost::json::value generate_id() {
        return boost::json::value(next_id_.fetch_add(1));
    }

    /**
     * @brief 同步调用
     */
    Response call(const Request& request) {
        auto session = create_session();
        return session->call(request);
    }

    /**
     * @brief 批量同步调用
     */
    std::vector<Response> call_batch(const std::vector<Request>& requests) {
        auto session = create_session();
        return session->call_batch(requests);
    }

    /**
     * @brief 异步调用
     */
    void async_call(const Request& request,
                   std::function<void(const Response&)> callback)
    {
        auto session = create_session();
        session->async_call(request, callback);
    }

    /**
     * @brief 发送通知
     */
    void notify(const Request& request) {
        auto session = create_session();
        session->notify(request);
    }

    void set_logger(std::function<void(const std::string&)> logger) {
        logger_ = std::move(logger);
    }

private:
    boost::asio::io_context io_context_;                ///< I/O 上下文
    std::string host_;                                  ///< 服务器地址
    std::string port_;                                  ///< 服务器端口
    std::chrono::milliseconds timeout_;                 ///< 超时时间
    std::atomic<int64_t> next_id_;                      ///< 下一个请求 ID
    std::function<void(const std::string&)> logger_;    ///< 日志回调
};

// ============================================================================
// Client 构造函数
// ============================================================================

inline Client::Client(const std::string& host, unsigned short port)
    : impl_(new Impl(host, port))
{
}

// ============================================================================
// Client 析构函数
// ============================================================================

inline Client::~Client() {
}

// ============================================================================
// 设置超时时间
// ============================================================================

inline void Client::set_timeout(std::chrono::milliseconds timeout) {
    impl_->set_timeout(timeout);
}

inline void Client::set_logger(std::function<void(const std::string&)> logger) {
    impl_->set_logger(std::move(logger));
}

// ============================================================================
// 同步调用（模板函数实现）
// ============================================================================

template<typename Result, typename... Args>
Result Client::call(const std::string& method, Args&&... args) {
    // 生成请求 ID
    boost::json::value id = impl_->generate_id();

    // 转换参数为 JSON
    boost::json::array params;
    int dummy[] = {0, (
        params.push_back(detail::json_converter<typename std::decay<Args>::type>::to_json(
            std::forward<Args>(args)
        )), 0)...};
    (void)dummy;  // 避免未使用警告

    // 构造请求
    Request request(method, params, id);

    // 同步调用
    Response response = impl_->call(request);

    // 检查错误
    if (response.is_error()) {
        throw response.error();
    }

    // 转换结果
    return detail::json_converter<Result>::from_json(response.result());
}

// ============================================================================
// 异步调用（模板函数实现）
// ============================================================================

template<typename... Args>
void Client::async_call(const std::string& method,
                       std::function<void(const Response&)> callback,
                       Args&&... args)
{
    // 生成请求 ID
    boost::json::value id = impl_->generate_id();

    // 转换参数为 JSON
    boost::json::array params;
    int dummy[] = {0, (
        params.push_back(detail::json_converter<typename std::decay<Args>::type>::to_json(
            std::forward<Args>(args)
        )), 0)...};
    (void)dummy;

    // 构造请求
    Request request(method, params, id);

    // 异步调用
    impl_->async_call(request, callback);
}

// ============================================================================
// 批量同步调用
// ============================================================================

inline std::vector<Response> Client::call_batch(const std::vector<Request>& requests) {
    return impl_->call_batch(requests);
}

// ============================================================================
// 发送通知（模板函数实现）
// ============================================================================

template<typename... Args>
void Client::notify(const std::string& method, Args&&... args) {
    // 转换参数为 JSON
    boost::json::array params;
    int dummy[] = {0, (
        params.push_back(detail::json_converter<typename std::decay<Args>::type>::to_json(
            std::forward<Args>(args)
        )), 0)...};
    (void)dummy;

    // 构造请求（无 ID）
    Request request(method, params);

    // 发送通知
    impl_->notify(request);
}

// ============================================================================
// 运行事件循环
// ============================================================================

inline void Client::run() {
    impl_->get_io_context().run();
}

// ============================================================================
// 轮询事件循环
// ============================================================================

inline std::size_t Client::poll() {
    return impl_->get_io_context().poll();
}

// ============================================================================
// 在限定时间内运行事件循环
// ============================================================================

inline std::size_t Client::run_for(std::chrono::steady_clock::duration duration) {
    return impl_->get_io_context().run_for(duration);
}

// ============================================================================
// 运行直到没有待处理事件
// ============================================================================

inline std::size_t Client::run_until_idle() {
    std::size_t total = 0;
    while (true) {
        std::size_t processed = impl_->get_io_context().poll();
        if (processed == 0) {
            break;
        }
        total += processed;
    }
    return total;
}

// ============================================================================
// 获取 io_context
// ============================================================================

inline boost::asio::io_context& Client::get_io_context() {
    return impl_->get_io_context();
}

} // namespace jsonrpc
