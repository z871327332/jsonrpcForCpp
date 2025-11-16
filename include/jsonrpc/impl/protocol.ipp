#pragma once

#include <jsonrpc/detail/protocol.hpp>
#include <jsonrpc/types.hpp>
#include <jsonrpc/errors.hpp>
#include <boost/json.hpp>

namespace jsonrpc {
namespace detail {

// ============================================================================
// 辅助函数：验证 JSON-RPC 版本
// ============================================================================

inline bool Protocol::validate_version(const boost::json::object& obj) {
    if (!obj.contains("jsonrpc")) {
        return false;
    }

    const auto& version = obj.at("jsonrpc");
    if (!version.is_string()) {
        return false;
    }

    return version.as_string() == "2.0";
}

// ============================================================================
// 辅助函数：判断是否为批量请求
// ============================================================================

inline bool Protocol::is_batch_request(const boost::json::value& jv) {
    return jv.is_array();
}

// ============================================================================
// 解析请求
// ============================================================================

inline std::vector<Request> Protocol::parse_request(const std::string& json_str) {
    // 解析 JSON 字符串
    boost::json::value jv;
    try {
        jv = boost::json::parse(json_str);
    } catch (const std::exception& e) {
        throw Error(ErrorCode::ParseError,
            std::string("JSON 解析失败: ") + e.what());
    }

    std::vector<Request> requests;

    // 检查是否为批量请求
    if (is_batch_request(jv)) {
        const auto& arr = jv.as_array();

        // 空的批量请求是无效的
        if (arr.empty()) {
            throw Error(ErrorCode::InvalidRequest, "批量请求不能为空");
        }

        // 解析每个请求
        requests.reserve(arr.size());
        for (const auto& elem : arr) {
            try {
                requests.push_back(Request::from_json(elem));
            } catch (const Error&) {
                // Request::from_json 已经抛出了合适的错误
                throw;
            }
        }
    } else {
        // 单个请求
        try {
            requests.push_back(Request::from_json(jv));
        } catch (const Error&) {
            throw;
        }
    }

    return requests;
}

// ============================================================================
// 序列化响应
// ============================================================================

inline std::string Protocol::serialize_response(const Response& response) {
    boost::json::object obj = response.to_json();
    return boost::json::serialize(obj);
}

inline std::string Protocol::serialize_batch_response(const std::vector<Response>& responses) {
    boost::json::array arr;
    arr.reserve(responses.size());

    for (const auto& response : responses) {
        arr.push_back(response.to_json());
    }

    return boost::json::serialize(arr);
}

// ============================================================================
// 序列化请求（客户端用）
// ============================================================================

inline std::string Protocol::serialize_request(const Request& request) {
    boost::json::object obj = request.to_json();
    return boost::json::serialize(obj);
}

inline std::string Protocol::serialize_batch_request(const std::vector<Request>& requests) {
    boost::json::array arr;
    arr.reserve(requests.size());

    for (const auto& request : requests) {
        arr.push_back(request.to_json());
    }

    return boost::json::serialize(arr);
}

// ============================================================================
// 解析响应（客户端用）
// ============================================================================

inline Response Protocol::parse_response(const std::string& json_str) {
    // 解析 JSON 字符串
    boost::json::value jv;
    try {
        jv = boost::json::parse(json_str);
    } catch (const std::exception& e) {
        throw Error(ErrorCode::ParseError,
            std::string("JSON 解析失败: ") + e.what());
    }

    // 响应必须是对象
    if (!jv.is_object()) {
        throw Error(ErrorCode::InvalidRequest, "响应必须是 JSON 对象");
    }

    // 解析响应对象
    try {
        return Response::from_json(jv.as_object());
    } catch (const Error&) {
        throw;
    }
}

inline std::vector<Response> Protocol::parse_batch_response(const std::string& json_str) {
    // 解析 JSON 字符串
    boost::json::value jv;
    try {
        jv = boost::json::parse(json_str);
    } catch (const std::exception& e) {
        throw Error(ErrorCode::ParseError,
            std::string("JSON 解析失败: ") + e.what());
    }

    // 批量响应必须是数组
    if (!jv.is_array()) {
        throw Error(ErrorCode::InvalidRequest, "批量响应必须是 JSON 数组");
    }

    const auto& arr = jv.as_array();
    std::vector<Response> responses;
    responses.reserve(arr.size());

    // 解析每个响应
    for (const auto& elem : arr) {
        if (!elem.is_object()) {
            throw Error(ErrorCode::InvalidRequest, "响应数组中的元素必须是对象");
        }

        try {
            responses.push_back(Response::from_json(elem.as_object()));
        } catch (const Error&) {
            throw;
        }
    }

    return responses;
}

} // namespace detail
} // namespace jsonrpc
