#pragma once

#include <jsonrpc/detail/method_registry.hpp>
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <memory>
#include <functional>

/**
 * @file server_session.hpp
 * @brief 服务端 HTTP 会话
 *
 * 处理单个客户端连接的 HTTP 请求和响应。
 *
 * @author 无事情小神仙
 */

namespace jsonrpc {
namespace detail {

/**
 * @brief 服务端会话
 *
 * 管理单个客户端连接，处理 HTTP 请求并返回响应。
 * 使用 shared_from_this 确保在异步操作期间对象保持有效。
 */
class ServerSession : public std::enable_shared_from_this<ServerSession> {
public:
    /**
     * @brief 构造会话
     *
     * @param socket TCP socket（移动语义）
     * @param registry 方法注册表（共享指针）
     */
    ServerSession(
        boost::asio::ip::tcp::socket socket,
        std::shared_ptr<MethodRegistry> registry,
        std::function<void(const std::string&)> logger
    );

    /**
     * @brief 启动会话
     *
     * 开始异步读取 HTTP 请求。
     */
    void start();

private:
    /**
     * @brief 异步读取 HTTP 请求
     */
    void do_read();

    /**
     * @brief 读取完成回调
     *
     * @param ec 错误码
     * @param bytes_transferred 传输字节数
     */
    void on_read(boost::beast::error_code ec, std::size_t bytes_transferred);

    /**
     * @brief 处理请求
     *
     * 解析 JSON-RPC 请求，调用方法，构造 HTTP 响应。
     */
    void process_request();

    /**
     * @brief 异步写入 HTTP 响应
     */
    void do_write();

    /**
     * @brief 写入完成回调
     *
     * @param ec 错误码
     * @param bytes_transferred 传输字节数
     * @param close 是否关闭连接
     */
    void on_write(boost::beast::error_code ec, std::size_t bytes_transferred, bool close);

    /**
     * @brief 关闭连接
     */
    void do_close();

    void log(const std::string& message) const;

    boost::beast::tcp_stream stream_;                                           ///< TCP 流
    boost::beast::flat_buffer buffer_;                                          ///< 读取缓冲区
    boost::beast::http::request<boost::beast::http::string_body> req_;          ///< HTTP 请求
    boost::beast::http::response<boost::beast::http::string_body> res_;         ///< HTTP 响应
    std::shared_ptr<MethodRegistry> registry_;                                  ///< 方法注册表
    std::function<void(const std::string&)> logger_;                            ///< 日志回调
};

} // namespace detail
} // namespace jsonrpc

// Header-only 模式下包含实现
#ifdef JSONRPC_HEADER_ONLY
#include <jsonrpc/impl/server_session.ipp>
#endif
