#pragma once

#include <jsonrpc/config.hpp>
#include <jsonrpc/types.hpp>
#include <jsonrpc/errors.hpp>
#include <boost/json.hpp>
#include <memory>
#include <string>
#include <functional>
#include <chrono>

/**
 * @file client.hpp
 * @brief JSON-RPC 客户端
 *
 * 提供基于 HTTP 的 JSON-RPC 2.0 客户端实现。
 *
 * @author 无事情小神仙
 */

// 前向声明
namespace boost {
namespace asio {
    class io_context;
}
}

namespace jsonrpc {

/**
 * @brief JSON-RPC 客户端
 *
 * 基于 HTTP 协议的 JSON-RPC 2.0 客户端实现。
 * 支持同步调用、异步调用、批量请求、通知。
 *
 * 使用示例：
 * @code
 * // 同步调用
 * jsonrpc::Client client("127.0.0.1", 8080);
 * auto result = client.call<int>("add", 1, 2);
 * std::cout << "Result: " << result << std::endl;
 *
 * // 异步调用
 * client.async_call<int>("add", [](const Response& response) {
 *     if (!response.is_error()) {
 *         std::cout << "Result: " << response.result().as_int64() << std::endl;
 *     }
 * }, 1, 2);
 * @endcode
 */
class JSONRPC_DECL Client {
public:
    /**
     * @brief 构造客户端
     * @param host 服务器地址（如 "127.0.0.1" 或 "example.com"）
     * @param port 服务器端口
     */
    Client(const std::string& host, unsigned short port);

    /**
     * @brief 析构函数
     */
    ~Client();

    // 禁止拷贝
    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;

    /**
     * @brief 设置请求超时时间
     * @param timeout 超时时间（毫秒）
     */
    void set_timeout(std::chrono::milliseconds timeout);

    /**
     * @brief 设置日志回调
     *
     * 用于输出网络错误、解析异常等调试信息。
     * 回调在 I/O 线程中触发，需要注意线程安全。
     *
     * @param logger 日志回调（可传入空函数以移除）
     */
    void set_logger(std::function<void(const std::string&)> logger);

    /**
     * @brief 同步调用 RPC 方法
     *
     * 阻塞直到收到响应或超时。
     * 支持自动参数类型转换。
     *
     * @tparam Result 返回值类型
     * @tparam Args 参数类型（可变参数）
     * @param method 方法名
     * @param args 方法参数
     * @return 返回值
     * @throws Error RPC 错误或网络错误
     *
     * @code
     * auto result = client.call<int>("add", 1, 2);
     * @endcode
     */
    template<typename Result, typename... Args>
    Result call(const std::string& method, Args&&... args);

    /**
     * @brief 异步调用 RPC 方法
     *
     * 立即返回，通过回调函数接收结果。
     * 需要调用 run() 或 poll() 处理响应。
     *
     * @tparam Args 参数类型（可变参数）
     * @param method 方法名
     * @param callback 回调函数 void(const Response&)
     * @param args 方法参数
     *
     * @code
     * client.async_call<int>("add", [](const Response& resp) {
     *     if (!resp.is_error()) {
     *         std::cout << resp.result().as_int64() << std::endl;
     *     }
     * }, 1, 2);
     * client.run();  // 处理响应
     * @endcode
     */
    template<typename... Args>
    void async_call(const std::string& method,
                    std::function<void(const Response&)> callback,
                    Args&&... args);

    /**
     * @brief 批量同步调用
     *
     * 一次发送多个请求，等待所有响应。
     *
     * @param requests 请求列表
     * @return 响应列表
     * @throws Error RPC 错误或网络错误
     */
    std::vector<Response> call_batch(const std::vector<Request>& requests);

    /**
     * @brief 发送通知（无响应）
     *
     * 发送通知类型的请求，不等待响应。
     *
     * @tparam Args 参数类型
     * @param method 方法名
     * @param args 方法参数
     *
     * @code
     * client.notify("log", "message");
     * @endcode
     */
    template<typename... Args>
    void notify(const std::string& method, Args&&... args);

    /**
     * @brief 运行事件循环（阻塞）
     *
     * 处理所有待处理的异步响应，直到所有操作完成。
     * 用于异步调用后等待响应。
     */
    void run();

    /**
     * @brief 轮询事件循环（非阻塞）
     *
     * 处理当前就绪的异步响应，立即返回。
     *
     * @return 处理的事件数量
     */
    std::size_t poll();

    /**
     * @brief 在限定时间内运行事件循环
     *
     * 运行事件循环至多 duration 时间，期间处理已就绪的异步响应。
     *
     * @param duration 最长运行时间
     * @return 处理的事件数量
     */
    std::size_t run_for(std::chrono::steady_clock::duration duration);

    /**
     * @brief 运行事件循环直到当前没有待处理事件
     *
     * 反复轮询，直到没有新的事件就绪为止。
     * 不会等待新的网络事件，仅处理已准备就绪的 handler。
     *
     * @return 处理的事件数量
     */
    std::size_t run_until_idle();

    /**
     * @brief 获取 io_context（高级用法）
     *
     * 允许用户访问底层的 Boost.Asio io_context，
     * 用于集成其他异步操作。
     *
     * @return io_context 引用
     */
    boost::asio::io_context& get_io_context();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace jsonrpc

// Header-only 模式下包含实现
#ifdef JSONRPC_HEADER_ONLY
#include <jsonrpc/impl/client.ipp>
#endif
