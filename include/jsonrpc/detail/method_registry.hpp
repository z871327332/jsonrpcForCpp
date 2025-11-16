#pragma once

#include <jsonrpc/detail/method_wrapper.hpp>
#include <jsonrpc/types.hpp>
#include <algorithm>
#include <atomic>
#include <boost/asio/post.hpp>
#include <boost/asio/thread_pool.hpp>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

/**
 * @file method_registry.hpp
 * @brief 方法注册表
 *
 * 管理 RPC 方法的注册和调用。
 *
 * @author 无事情小神仙
 */

namespace jsonrpc {
namespace detail {

/**
 * @brief 方法注册表
 *
 * 存储已注册的 RPC 方法，提供线程安全的注册和调用接口。
 */
class MethodRegistry {
public:
    /**
     * @brief 默认构造函数
     */
    MethodRegistry();

    /**
     * @brief 设置批量请求并行度
     *
     * @param threads 并行线程数，最小为 1
     */
    void set_batch_concurrency(std::size_t threads);

    /**
     * @brief 注册方法
     *
     * @tparam Func 函数类型
     * @param name 方法名
     * @param func 函数对象
     */
    template<typename Func>
    void register_method(const std::string& name, Func&& func);

    /**
     * @brief 调用方法
     *
     * @param request 请求对象
     * @return 响应对象
     */
    Response invoke(const Request& request);

    /**
     * @brief 批量调用方法
     *
     * @param requests 请求对象列表
     * @return 响应对象列表
     */
    std::vector<Response> invoke_batch(const std::vector<Request>& requests);

private:
    std::shared_ptr<boost::asio::thread_pool> get_batch_pool();

    std::map<std::string, std::shared_ptr<MethodWrapperBase>> methods_;
    std::mutex mutex_;  ///< 保护 methods_ 的并发访问
    std::size_t batch_thread_count_;
    std::shared_ptr<boost::asio::thread_pool> batch_pool_;
    std::mutex pool_mutex_;
};

} // namespace detail
} // namespace jsonrpc

// Header-only 模式下包含实现
#ifdef JSONRPC_HEADER_ONLY
#include <jsonrpc/impl/method_registry.ipp>
#endif
