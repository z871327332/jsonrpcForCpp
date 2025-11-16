#pragma once

#include <jsonrpc/detail/server_session.hpp>
#include <jsonrpc/detail/protocol.hpp>
#include <jsonrpc/errors.hpp>

namespace jsonrpc {
namespace detail {

// ============================================================================
// 构造函数
// ============================================================================

inline ServerSession::ServerSession(
    boost::asio::ip::tcp::socket socket,
    std::shared_ptr<MethodRegistry> registry,
    std::function<void(const std::string&)> logger)
    : stream_(std::move(socket))
    , registry_(std::move(registry))
    , logger_(std::move(logger))
{
}

inline void ServerSession::log(const std::string& message) const {
    if (logger_) {
        logger_(message);
    }
}

// ============================================================================
// 启动会话
// ============================================================================

inline void ServerSession::start() {
    do_read();
}

// ============================================================================
// 异步读取 HTTP 请求
// ============================================================================

inline void ServerSession::do_read() {
    // 清空请求对象
    req_ = {};

    // 设置超时（30 秒）
    stream_.expires_after(std::chrono::seconds(30));

    // 异步读取 HTTP 请求
    auto self = shared_from_this();
    boost::beast::http::async_read(
        stream_,
        buffer_,
        req_,
        [self](boost::beast::error_code ec, std::size_t bytes_transferred) {
            self->on_read(ec, bytes_transferred);
        }
    );
}

// ============================================================================
// 读取完成回调
// ============================================================================

inline void ServerSession::on_read(boost::beast::error_code ec, std::size_t /*bytes_transferred*/) {
    // 连接关闭
    if (ec == boost::beast::http::error::end_of_stream) {
        do_close();
        return;
    }

    // 其他错误
    if (ec) {
        // 忽略错误，关闭连接
        log(std::string("读取请求失败: ") + ec.message());
        return;
    }

    // 处理请求
    process_request();
}

// ============================================================================
// 处理请求
// ============================================================================

inline void ServerSession::process_request() {
    // 验证 HTTP 方法（必须是 POST）
    if (req_.method() != boost::beast::http::verb::post) {
        log("收到非 POST 请求");
        res_ = {};
        res_.result(boost::beast::http::status::method_not_allowed);
        res_.set(boost::beast::http::field::content_type, "text/plain");
        res_.body() = "仅支持 POST 方法";
        res_.prepare_payload();
        do_write();
        return;
    }

    // 验证 Content-Type（应该是 application/json）
    auto content_type = req_[boost::beast::http::field::content_type];
    if (content_type.find("application/json") == std::string::npos) {
        log("Content-Type 无效: " + std::string(content_type));
        res_ = {};
        res_.result(boost::beast::http::status::unsupported_media_type);
        res_.set(boost::beast::http::field::content_type, "text/plain");
        res_.body() = "Content-Type 必须是 application/json";
        res_.prepare_payload();
        do_write();
        return;
    }

    // 提取请求 body
    std::string request_body = req_.body();

    // 解析 JSON-RPC 请求
    std::vector<Request> requests;
    bool is_batch = false;
    try {
        requests = Protocol::parse_request(request_body);
        is_batch = (requests.size() > 1) || Protocol::is_batch_request(boost::json::parse(request_body));
    } catch (const Error& e) {
        // 解析错误，返回错误响应
        log(std::string("解析请求失败: ") + e.what());
        Response error_response(e, boost::json::value(nullptr));
        res_ = {};
        res_.result(boost::beast::http::status::ok);
        res_.set(boost::beast::http::field::content_type, "application/json");
        res_.body() = Protocol::serialize_response(error_response);
        res_.prepare_payload();
        do_write();
        return;
    }

    // 调用方法
    std::vector<Response> responses = registry_->invoke_batch(requests);

    // 构造 HTTP 响应
    res_ = {};
    res_.result(boost::beast::http::status::ok);
    res_.set(boost::beast::http::field::content_type, "application/json");

    if (is_batch) {
        // 批量响应
        res_.body() = Protocol::serialize_batch_response(responses);
    } else {
        // 单个响应
        if (!responses.empty()) {
            res_.body() = Protocol::serialize_response(responses[0]);
        } else {
            // 通知类型的请求，无响应（返回 204 No Content）
            res_.result(boost::beast::http::status::no_content);
        }
    }

    res_.prepare_payload();

    // 设置 Keep-Alive
    res_.keep_alive(req_.keep_alive());

    do_write();
}

// ============================================================================
// 异步写入 HTTP 响应
// ============================================================================

inline void ServerSession::do_write() {
    bool close = !res_.keep_alive();

    auto self = shared_from_this();
    boost::beast::http::async_write(
        stream_,
        res_,
        [self, close](boost::beast::error_code ec, std::size_t bytes_transferred) {
            self->on_write(ec, bytes_transferred, close);
        }
    );
}

// ============================================================================
// 写入完成回调
// ============================================================================

inline void ServerSession::on_write(boost::beast::error_code ec, std::size_t /*bytes_transferred*/, bool close) {
    if (ec) {
        // 写入错误，关闭连接
        log(std::string("写入响应失败: ") + ec.message());
        return;
    }

    if (close) {
        // 需要关闭连接
        do_close();
        return;
    }

    // Keep-Alive，继续读取下一个请求
    do_read();
}

// ============================================================================
// 关闭连接
// ============================================================================

inline void ServerSession::do_close() {
    boost::beast::error_code ec;
    stream_.socket().shutdown(boost::asio::ip::tcp::socket::shutdown_send, ec);
    // 忽略错误
}

} // namespace detail
} // namespace jsonrpc
