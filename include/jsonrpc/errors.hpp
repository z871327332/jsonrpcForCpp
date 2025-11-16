#pragma once

#include <jsonrpc/config.hpp>
#include <boost/json.hpp>
#include <exception>
#include <string>

namespace jsonrpc {

/**
 * @brief JSON-RPC 2.0 标准错误码
 */
enum class ErrorCode : int {
    ParseError = -32700,      ///< 解析错误：无效的 JSON
    InvalidRequest = -32600,  ///< 无效的请求：不符合 JSON-RPC 2.0 规范
    MethodNotFound = -32601,  ///< 方法未找到
    InvalidParams = -32602,   ///< 无效的参数
    InternalError = -32603,   ///< 内部错误
    ServerError = -32000      ///< 服务端错误（-32000 到 -32099）
};

/**
 * @brief JSON-RPC 错误类
 *
 * 表示 JSON-RPC 2.0 错误对象，继承自 std::exception。
 * 包含错误码、错误消息和可选的附加数据。
 *
 * @author 无事情小神仙
 */
class Error : public std::exception {
public:
    /**
     * @brief 构造错误对象
     * @param code 错误码
     * @param message 错误消息
     */
    Error(ErrorCode code, std::string message)
        : code_(code)
        , message_(std::move(message))
        , data_(nullptr)
        , what_str_(format_what())
    {}

    /**
     * @brief 构造错误对象（带附加数据）
     * @param code 错误码
     * @param message 错误消息
     * @param data 附加数据（JSON 值）
     */
    Error(ErrorCode code, std::string message, boost::json::value data)
        : code_(code)
        , message_(std::move(message))
        , data_(std::move(data))
        , what_str_(format_what())
    {}

    /**
     * @brief 获取错误码
     * @return 错误码
     */
    ErrorCode code() const noexcept {
        return code_;
    }

    /**
     * @brief 获取错误消息（std::exception 接口）
     * @return 错误消息 C 字符串
     */
    const char* what() const noexcept override {
        return what_str_.c_str();
    }

    /**
     * @brief 获取错误消息
     * @return 错误消息字符串
     */
    const std::string& message() const noexcept {
        return message_;
    }

    /**
     * @brief 获取附加数据
     * @return 附加数据（JSON 值）
     */
    const boost::json::value& data() const noexcept {
        return data_;
    }

    /**
     * @brief 检查是否有附加数据
     * @return 如果有附加数据返回 true
     */
    bool has_data() const noexcept {
        return !data_.is_null();
    }

    /**
     * @brief 转换为 JSON 对象
     * @return JSON-RPC 错误对象
     */
    boost::json::object to_json() const {
        boost::json::object obj;
        obj["code"] = static_cast<int>(code_);
        obj["message"] = message_;
        if (!data_.is_null()) {
            obj["data"] = data_;
        }
        return obj;
    }

private:
    std::string format_what() const {
        return "JSON-RPC Error [" + std::to_string(static_cast<int>(code_)) + "]: " + message_;
    }

    ErrorCode code_;
    std::string message_;
    boost::json::value data_;
    std::string what_str_;
};

} // namespace jsonrpc
