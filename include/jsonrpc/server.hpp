#pragma once

#include <jsonrpc/config.hpp>
#include <jsonrpc/types.hpp>
#include <jsonrpc/errors.hpp>
#include <memory>
#include <stdexcept>
#include <string>

/**
 * @file server.hpp
 * @brief JSON-RPC 服务端
 *
 * 提供基于 HTTP 的 JSON-RPC 2.0 服务端实现。
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
 * @brief JSON-RPC 服务端
 *
 * 基于 HTTP 协议的 JSON-RPC 2.0 服务端实现。
 * 支持方法注册、异步处理、批量请求。
 *
 * 使用示例：
 * @code
 * jsonrpc::Server server(8080);
 * server.register_method("add", [](int a, int b) { return a + b; });
 * server.run();  // 阻塞运行
 * @endcode
 */
class JSONRPC_DECL Server {
public:
    /**
     * @brief 构造服务端（监听所有接口）
     * @param port 监听端口
     * @throws boost::system::system_error 端口被占用或无法绑定
     */
    explicit Server(unsigned short port);

    /**
     * @brief 构造服务端（指定监听地址）
     * @param port 监听端口
     * @param address 监听地址（如 "127.0.0.1" 或 "0.0.0.0"）
     * @throws boost::system::system_error 地址格式错误、端口被占用或无法绑定
     */
    Server(unsigned short port, const std::string& address);

    /**
     * @brief 析构函数
     */
    ~Server();

    // 禁止拷贝
    Server(const Server&) = delete;
    Server& operator=(const Server&) = delete;

    /**
     * @brief 注册 RPC 方法
     *
     * 支持任意函数签名，参数和返回值会自动转换。
     *
     * @tparam Func 函数类型（函数指针、lambda、std::function 等）
     * @param name 方法名
     * @param func 函数对象
     *
     * @code
     * // 普通函数
     * int add(int a, int b) { return a + b; }
     * server.register_method("add", add);
     *
     * // Lambda
     * server.register_method("subtract", [](int a, int b) { return a - b; });
     *
     * // std::function
     * std::function<int(int, int)> multiply = [](int a, int b) { return a * b; };
     * server.register_method("multiply", multiply);
     * @endcode
     */
    template<typename Func>
    void register_method(const std::string& name, Func&& func);

    /**
     * @brief 运行服务器（阻塞）
     *
     * 阻塞当前线程，开始处理请求。
     * 调用 stop() 可以停止服务器。
     */
    void run();

    /**
     * @brief 异步启动服务器（非阻塞）
     *
     * 在后台线程运行服务器，立即返回。
     * 需要调用 stop() 停止服务器并等待线程结束。
     */
    void start();

    /**
     * @brief 停止服务器
     *
     * 停止接受新连接，关闭现有连接。
     * 如果服务器是通过 start() 启动的，会等待后台线程结束。
     */
    void stop();

    /**
     * @brief 获取 io_context（高级用法）
     *
     * 允许用户访问底层的 Boost.Asio io_context，
     * 用于集成其他异步操作。
     *
     * @return io_context 引用
     */
    boost::asio::io_context& get_io_context();

    /**
     * @brief 判断服务器是否正在运行
     *
     * @return 如果服务器已经通过 run()/start() 启动且尚未 stop()，返回 true
     */
    bool is_running() const;

    /**
     * @brief 设置批量请求处理的并行线程数
     *
     * @param threads 并行线程数，最小为 1
     * @throws std::logic_error 当服务器正在运行时调用
     */
    void set_batch_concurrency(std::size_t threads);

    /**
     * @brief 设置日志回调
     *
     * 用于捕获网络错误、无效请求等调试信息。
     * 回调会在 I/O 线程执行，需要注意线程安全。
     *
     * @param logger 日志回调（传入空函数可移除）
     */
    void set_logger(std::function<void(const std::string&)> logger);

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace jsonrpc

// Header-only 模式下包含实现
#ifdef JSONRPC_HEADER_ONLY
#include <jsonrpc/impl/server.ipp>
#endif
