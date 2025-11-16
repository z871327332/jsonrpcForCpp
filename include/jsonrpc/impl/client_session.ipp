#pragma once

#include <jsonrpc/detail/client_session.hpp>
#include <jsonrpc/detail/protocol.hpp>
#include <jsonrpc/errors.hpp>

namespace jsonrpc {
namespace detail {

// ============================================================================
// 构造函数
// ============================================================================

inline ClientSession::ClientSession(
    boost::asio::io_context& io_context,
    const std::string& host,
    const std::string& port,
    std::chrono::milliseconds timeout,
    std::function<void(const std::string&)> logger)
    : io_context_(io_context)
    , resolver_(io_context)
    , stream_(io_context)
    , host_(host)
    , port_(port)
    , timeout_(timeout)
    , logger_(std::move(logger))
{
}

inline void ClientSession::log(const std::string& message) const {
    if (logger_) {
        logger_(message);
    }
}

// ============================================================================
// 同步调用
// ============================================================================

inline Response ClientSession::call(const Request& request) {
    // 序列化请求
    std::string request_body = Protocol::serialize_request(request);

    // 发送请求并接收响应
    std::string response_body = send_request_sync(request_body);

    // 解析响应
    try {
        return Protocol::parse_response(response_body);
    } catch (const Error& e) {
        log(std::string("解析响应失败: ") + e.what());
        throw;
    }
}

// ============================================================================
// 批量同步调用
// ============================================================================

inline std::vector<Response> ClientSession::call_batch(const std::vector<Request>& requests) {
    // 序列化批量请求
    std::string request_body = Protocol::serialize_batch_request(requests);

    // 发送请求并接收响应
    std::string response_body = send_request_sync(request_body);

    // 解析批量响应
    try {
        return Protocol::parse_batch_response(response_body);
    } catch (const Error& e) {
        log(std::string("解析批量响应失败: ") + e.what());
        throw;
    }
}

// ============================================================================
// 异步调用
// ============================================================================

inline void ClientSession::async_call(const Request& request,
                                     std::function<void(const Response&)> callback)
{
    // 序列化请求
    std::string request_body = Protocol::serialize_request(request);

    // 异步发送请求
    auto self = shared_from_this();
    send_request_async(request_body, [self, callback](boost::beast::error_code ec, const std::string& response_body) {
        if (ec) {
            // 网络错误，转换为 RPC 错误响应
            Error error(ErrorCode::InternalError,
                       std::string("网络错误: ") + ec.message());
            Response error_response(error, boost::json::value(nullptr));
            callback(error_response);
            return;
        }

        try {
            // 解析响应
            Response response = Protocol::parse_response(response_body);
            callback(response);
        } catch (const Error& e) {
            // 解析错误，创建错误响应
            self->log(std::string("解析响应失败: ") + e.what());
            Response error_response(e, boost::json::value(nullptr));
            callback(error_response);
        }
    });
}

// ============================================================================
// 发送通知
// ============================================================================

inline void ClientSession::notify(const Request& request) {
    // 序列化请求
    std::string request_body = Protocol::serialize_request(request);

    // 发送请求（不等待响应）
    try {
        send_request_sync(request_body);
    } catch (...) {
        // 通知类型的请求，忽略错误
    }
}

// ============================================================================
// 同步发送请求并接收响应
// ============================================================================

inline std::string ClientSession::send_request_sync(const std::string& request_body) {
    try {
        // 解析域名
        auto const results = resolver_.resolve(host_, port_);

        // 设置超时
        stream_.expires_after(timeout_);

        // 连接到服务器
        stream_.connect(results);

        // 构造 HTTP 请求
        req_ = {};
        req_.version(11);  // HTTP/1.1
        req_.method(boost::beast::http::verb::post);
        req_.target("/");
        req_.set(boost::beast::http::field::host, host_);
        req_.set(boost::beast::http::field::content_type, "application/json");
        req_.set(boost::beast::http::field::user_agent, "jsonrpc-client");
        req_.body() = request_body;
        req_.prepare_payload();

        // 发送 HTTP 请求
        stream_.expires_after(timeout_);
        boost::beast::http::write(stream_, req_);

        // 接收 HTTP 响应
        buffer_ = {};
        res_ = {};
        stream_.expires_after(timeout_);
        boost::beast::http::read(stream_, buffer_, res_);

        // 提取响应 body
        std::string response_body = res_.body();

        // 优雅关闭连接
        boost::beast::error_code ec;
        stream_.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);

        return response_body;

    } catch (const boost::system::system_error& e) {
        // 网络错误
        log(std::string("网络错误: ") + e.what());
        throw Error(ErrorCode::InternalError,
                   std::string("网络错误: ") + e.what());
    }
}

// ============================================================================
// 异步发送请求
// ============================================================================

inline void ClientSession::send_request_async(const std::string& request_body,
                                              std::function<void(boost::beast::error_code, const std::string&)> callback)
{
    // 构造 HTTP 请求
    req_ = {};
    req_.version(11);
    req_.method(boost::beast::http::verb::post);
    req_.target("/");
    req_.set(boost::beast::http::field::host, host_);
    req_.set(boost::beast::http::field::content_type, "application/json");
    req_.set(boost::beast::http::field::user_agent, "jsonrpc-client");
    req_.body() = request_body;
    req_.prepare_payload();

    // 异步连接
    auto self = shared_from_this();
    do_connect([self, callback](boost::beast::error_code ec) {
        if (ec) {
            // 连接错误，直接传递错误码
            self->log(std::string("连接失败: ") + ec.message());
            callback(ec, "");
            return;
        }

        // 异步写入
        self->do_write([self, callback](boost::beast::error_code ec) {
            if (ec) {
                // 写入错误，直接传递错误码
                self->log(std::string("写入请求失败: ") + ec.message());
                callback(ec, "");
                return;
            }

            // 异步读取
            self->do_read([self, callback](boost::beast::error_code ec) {
                if (ec) {
                    // 读取错误，直接传递错误码
                    self->log(std::string("读取响应失败: ") + ec.message());
                    callback(ec, "");
                    return;
                }

                // 提取响应 body
                std::string response_body = self->res_.body();

                // 关闭连接
                boost::beast::error_code close_ec;
                self->stream_.socket().shutdown(
                    boost::asio::ip::tcp::socket::shutdown_both,
                    close_ec
                );

                // 成功，传递空错误码和响应字符串
                callback(boost::beast::error_code(), response_body);
            });
        });
    });
}

// ============================================================================
// 异步连接
// ============================================================================

inline void ClientSession::do_connect(std::function<void(boost::beast::error_code)> callback) {
    // 异步解析域名
    auto self = shared_from_this();
    resolver_.async_resolve(host_, port_,
        [self, callback](boost::beast::error_code ec,
                        boost::asio::ip::tcp::resolver::results_type results) {
            if (ec) {
                self->log(std::string("解析域名失败: ") + ec.message());
                callback(ec);
                return;
            }

            // 设置超时
            self->stream_.expires_after(self->timeout_);

            // 异步连接
            self->stream_.async_connect(results,
                [self, callback](boost::beast::error_code ec,
                          boost::asio::ip::tcp::resolver::results_type::endpoint_type) {
                    if (ec) {
                        self->log(std::string("连接失败: ") + ec.message());
                    }
                    callback(ec);
                }
            );
        }
    );
}

// ============================================================================
// 异步写入请求
// ============================================================================

inline void ClientSession::do_write(std::function<void(boost::beast::error_code)> callback) {
    // 设置超时
    stream_.expires_after(timeout_);

    // 异步写入
    auto self = shared_from_this();
    boost::beast::http::async_write(stream_, req_,
        [self, callback](boost::beast::error_code ec, std::size_t) {
            if (ec) {
                self->log(std::string("写入请求失败: ") + ec.message());
            }
            callback(ec);
        }
    );
}

// ============================================================================
// 异步读取响应
// ============================================================================

inline void ClientSession::do_read(std::function<void(boost::beast::error_code)> callback) {
    // 设置超时
    stream_.expires_after(timeout_);

    // 清空缓冲区
    buffer_ = {};
    res_ = {};

    // 异步读取
    auto self = shared_from_this();
    boost::beast::http::async_read(stream_, buffer_, res_,
        [self, callback](boost::beast::error_code ec, std::size_t) {
            if (ec) {
                self->log(std::string("读取响应失败: ") + ec.message());
            }
            callback(ec);
        }
    );
}

} // namespace detail
} // namespace jsonrpc
