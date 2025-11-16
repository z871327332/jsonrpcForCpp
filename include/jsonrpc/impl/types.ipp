#pragma once

#include <jsonrpc/types.hpp>
#include <jsonrpc/errors.hpp>

namespace jsonrpc {

// ============================================================================
// Request 实现
// ============================================================================

inline Request::Request()
    : method_()
    , params_(nullptr)
    , id_(nullptr)
    , has_id_(false)
{}

inline Request::Request(std::string method, boost::json::value params, boost::json::value id)
    : method_(std::move(method))
    , params_(std::move(params))
    , id_(std::move(id))
    , has_id_(true)
{}

inline Request::Request(std::string method, boost::json::value params)
    : method_(std::move(method))
    , params_(std::move(params))
    , id_(nullptr)
    , has_id_(false)
{}

inline const std::string& Request::method() const {
    return method_;
}

inline const boost::json::value& Request::params() const {
    return params_;
}

inline const boost::json::value& Request::id() const {
    return id_;
}

inline bool Request::has_id() const {
    return has_id_;
}

inline Request Request::from_json(const boost::json::value& jv) {
    if (!jv.is_object()) {
        throw Error(ErrorCode::InvalidRequest, "请求必须是 JSON 对象");
    }

    const auto& obj = jv.as_object();

    // 验证 jsonrpc 版本
    if (!obj.contains("jsonrpc") || !obj.at("jsonrpc").is_string() ||
        obj.at("jsonrpc").as_string() != "2.0") {
        throw Error(ErrorCode::InvalidRequest, "缺少或无效的 jsonrpc 版本字段");
    }

    // 提取 method
    if (!obj.contains("method") || !obj.at("method").is_string()) {
        throw Error(ErrorCode::InvalidRequest, "缺少或无效的 method 字段");
    }
    std::string method(obj.at("method").as_string().c_str());

    // 提取 params（可选）
    boost::json::value params = nullptr;
    if (obj.contains("params")) {
        params = obj.at("params");
        // params 必须是 array 或 object
        if (!params.is_array() && !params.is_object() && !params.is_null()) {
            throw Error(ErrorCode::InvalidRequest, "params 必须是 array 或 object");
        }
    }

    // 提取 id（可选，通知没有 id）
    if (obj.contains("id")) {
        const auto& id_val = obj.at("id");
        // id 必须是 string、number 或 null
        if (!id_val.is_string() && !id_val.is_number() && !id_val.is_null()) {
            throw Error(ErrorCode::InvalidRequest, "id 必须是 string、number 或 null");
        }
        return Request(std::move(method), std::move(params), id_val);
    } else {
        // 通知（没有 id）
        return Request(std::move(method), std::move(params));
    }
}

inline boost::json::object Request::to_json() const {
    boost::json::object obj;
    obj["jsonrpc"] = "2.0";
    obj["method"] = method_;

    if (!params_.is_null()) {
        obj["params"] = params_;
    }

    if (has_id_) {
        obj["id"] = id_;
    }

    return obj;
}

// ============================================================================
// Response 实现
// ============================================================================

inline Response::Response(boost::json::value result, boost::json::value id)
    : is_error_(false)
    , result_(std::move(result))
    , error_(ErrorCode::InternalError, "")  // 占位符，不会被使用
    , id_(std::move(id))
{}

inline Response::Response(const Error& error, boost::json::value id)
    : is_error_(true)
    , result_(nullptr)
    , error_(error)
    , id_(std::move(id))
{}

inline bool Response::is_error() const {
    return is_error_;
}

inline const boost::json::value& Response::result() const {
    if (is_error_) {
        throw std::logic_error("错误响应没有 result");
    }
    return result_;
}

inline const Error& Response::error() const {
    if (!is_error_) {
        throw std::logic_error("成功响应没有 error");
    }
    return error_;
}

inline const boost::json::value& Response::id() const {
    return id_;
}

inline Response Response::from_json(const boost::json::value& jv) {
    if (!jv.is_object()) {
        throw Error(ErrorCode::InvalidRequest, "响应必须是 JSON 对象");
    }

    const auto& obj = jv.as_object();

    // 验证 jsonrpc 版本
    if (!obj.contains("jsonrpc") || !obj.at("jsonrpc").is_string() ||
        obj.at("jsonrpc").as_string() != "2.0") {
        throw Error(ErrorCode::InvalidRequest, "缺少或无效的 jsonrpc 版本字段");
    }

    // 提取 id
    if (!obj.contains("id")) {
        throw Error(ErrorCode::InvalidRequest, "缺少 id 字段");
    }
    const auto& id = obj.at("id");

    // 检查是成功响应还是错误响应
    bool has_result = obj.contains("result");
    bool has_error = obj.contains("error");

    if (has_result && has_error) {
        throw Error(ErrorCode::InvalidRequest, "响应不能同时包含 result 和 error");
    }

    if (!has_result && !has_error) {
        throw Error(ErrorCode::InvalidRequest, "响应必须包含 result 或 error");
    }

    if (has_result) {
        // 成功响应
        return Response(obj.at("result"), id);
    } else {
        // 错误响应
        const auto& error_obj = obj.at("error");
        if (!error_obj.is_object()) {
            throw Error(ErrorCode::InvalidRequest, "error 必须是对象");
        }

        const auto& err = error_obj.as_object();
        if (!err.contains("code") || !err.at("code").is_int64()) {
            throw Error(ErrorCode::InvalidRequest, "error.code 缺失或无效");
        }
        if (!err.contains("message") || !err.at("message").is_string()) {
            throw Error(ErrorCode::InvalidRequest, "error.message 缺失或无效");
        }

        int code = static_cast<int>(err.at("code").as_int64());
        std::string message(err.at("message").as_string().c_str());

        Error error(static_cast<ErrorCode>(code), std::move(message));
        if (err.contains("data")) {
            error = Error(static_cast<ErrorCode>(code), message, err.at("data"));
        }

        return Response(error, id);
    }
}

inline boost::json::object Response::to_json() const {
    boost::json::object obj;
    obj["jsonrpc"] = "2.0";

    if (is_error_) {
        obj["error"] = error_.to_json();
    } else {
        obj["result"] = result_;
    }

    obj["id"] = id_;

    return obj;
}

} // namespace jsonrpc
