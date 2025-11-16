#pragma once

#include <jsonrpc/types.hpp>
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <memory>
#include <string>
#include <functional>
#include <chrono>

/**
 * @file client_session.hpp
 * @brief 客户端 HTTP 会话
 *
 * 处理单个 HTTP 请求/响应的生命周期。
 *
 * @author 无事情小神仙
 */

namespace jsonrpc {
namespace detail {

/**
 * @brief 客户端会话
 *
 * 管理单个 HTTP 请求/响应，支持同步和异步操作。
 * 使用 shared_from_this 确保异步操作期间对象有效。
 */
class ClientSession : public std::enable_shared_from_this<ClientSession> {
public:
    /**
     * @brief 构造会话
     *
     * @param io_context I/O 上下文
     * @param host 服务器地址
     * @param port 服务器端口
     * @param timeout 超时时间
     */
    ClientSession(
        boost::asio::io_context& io_context,
        const std::string& host,
        const std::string& port,
        std::chrono::milliseconds timeout,
        std::function<void(const std::string&)> logger
    );

    /**
     * @brief 同步调用
     *
     * @param request 请求对象
     * @return 响应对象
     * @throws Error 网络错误或 RPC 错误
     */
    Response call(const Request& request);

    /**
     * @brief 批量同步调用
     *
     * @param requests 请求列表
     * @return 响应列表
     * @throws Error 网络错误或 RPC 错误
     */
    std::vector<Response> call_batch(const std::vector<Request>& requests);

    /**
     * @brief 异步调用
     *
     * @param request 请求对象
     * @param callback 回调函数
     */
    void async_call(const Request& request,
                    std::function<void(const Response&)> callback);

    /**
     * @brief 发送通知（无响应）
     *
     * @param request 请求对象（无 ID）
     */
    void notify(const Request& request);

private:
    /**
     * @brief 同步发送请求并接收响应
     *
     * @param request_body 请求 body（JSON 字符串）
     * @return 响应 body（JSON 字符串）
     */
    std::string send_request_sync(const std::string& request_body);

    /**
     * @brief 异步发送请求
     *
     * @param request_body 请求 body
     * @param callback 回调函数（error_code, 响应字符串）
     */
    void send_request_async(const std::string& request_body,
                           std::function<void(boost::beast::error_code, const std::string&)> callback);

    /**
     * @brief 异步连接
     */
    void do_connect(std::function<void(boost::beast::error_code)> callback);

    /**
     * @brief 异步写入请求
     */
    void do_write(std::function<void(boost::beast::error_code)> callback);

    /**
     * @brief 异步读取响应
     */
    void do_read(std::function<void(boost::beast::error_code)> callback);

    void log(const std::string& message) const;

    boost::asio::io_context& io_context_;                       ///< I/O 上下文
    boost::asio::ip::tcp::resolver resolver_;                   ///< DNS 解析器
    boost::beast::tcp_stream stream_;                           ///< TCP 流
    std::string host_;                                          ///< 服务器地址
    std::string port_;                                          ///< 服务器端口
    std::chrono::milliseconds timeout_;                         ///< 超时时间
    std::function<void(const std::string&)> logger_;             ///< 日志回调

    boost::beast::flat_buffer buffer_;                          ///< 读取缓冲区
    boost::beast::http::request<boost::beast::http::string_body> req_;   ///< HTTP 请求
    boost::beast::http::response<boost::beast::http::string_body> res_;  ///< HTTP 响应
};

} // namespace detail
} // namespace jsonrpc

// Header-only 模式下包含实现
#ifdef JSONRPC_HEADER_ONLY
#include <jsonrpc/impl/client_session.ipp>
#endif
