#pragma once

#include <jsonrpc/config.hpp>
#include <jsonrpc/errors.hpp>
#include <boost/json.hpp>
#include <string>

namespace jsonrpc {

/**
 * @brief JSON-RPC 请求对象
 *
 * 表示 JSON-RPC 2.0 请求，包含方法名、参数和请求 ID。
 * 通知类型的请求没有 ID。
 *
 * @author 无事情小神仙
 */
class JSONRPC_DECL Request {
public:
    /**
     * @brief 默认构造函数
     */
    Request();

    /**
     * @brief 构造请求对象
     * @param method 方法名
     * @param params 参数（JSON array 或 object）
     * @param id 请求 ID（字符串、数字或 null）
     */
    Request(std::string method, boost::json::value params, boost::json::value id);

    /**
     * @brief 构造通知对象（无 ID）
     * @param method 方法名
     * @param params 参数
     */
    Request(std::string method, boost::json::value params);

    /**
     * @brief 获取方法名
     * @return 方法名
     */
    const std::string& method() const;

    /**
     * @brief 获取参数
     * @return 参数（JSON 值）
     */
    const boost::json::value& params() const;

    /**
     * @brief 获取请求 ID
     * @return 请求 ID
     */
    const boost::json::value& id() const;

    /**
     * @brief 检查是否有 ID（区分请求和通知）
     * @return 如果有 ID 返回 true，通知返回 false
     */
    bool has_id() const;

    /**
     * @brief 从 JSON 值解析请求
     * @param jv JSON 值
     * @return Request 对象
     * @throws Error 如果解析失败
     */
    static Request from_json(const boost::json::value& jv);

    /**
     * @brief 转换为 JSON 对象
     * @return JSON-RPC 请求对象
     */
    boost::json::object to_json() const;

private:
    std::string method_;
    boost::json::value params_;
    boost::json::value id_;
    bool has_id_;
};

/**
 * @brief JSON-RPC 响应对象
 *
 * 表示 JSON-RPC 2.0 响应，可以是成功响应（包含 result）
 * 或错误响应（包含 error）。
 *
 * @author 无事情小神仙
 */
class JSONRPC_DECL Response {
public:
    /**
     * @brief 构造成功响应
     * @param result 结果（JSON 值）
     * @param id 对应请求的 ID
     */
    Response(boost::json::value result, boost::json::value id);

    /**
     * @brief 构造错误响应
     * @param error 错误对象
     * @param id 对应请求的 ID
     */
    Response(const Error& error, boost::json::value id);

    /**
     * @brief 检查是否为错误响应
     * @return 如果是错误响应返回 true
     */
    bool is_error() const;

    /**
     * @brief 获取结果（仅成功响应有效）
     * @return 结果（JSON 值）
     */
    const boost::json::value& result() const;

    /**
     * @brief 获取错误对象（仅错误响应有效）
     * @return 错误对象
     */
    const Error& error() const;

    /**
     * @brief 获取响应 ID
     * @return 响应 ID
     */
    const boost::json::value& id() const;

    /**
     * @brief 从 JSON 值解析响应
     * @param jv JSON 值
     * @return Response 对象
     * @throws Error 如果解析失败
     */
    static Response from_json(const boost::json::value& jv);

    /**
     * @brief 转换为 JSON 对象
     * @return JSON-RPC 响应对象
     */
    boost::json::object to_json() const;

private:
    bool is_error_;
    boost::json::value result_;
    Error error_;
    boost::json::value id_;
};

} // namespace jsonrpc

// Header-only 模式下包含实现
#ifdef JSONRPC_HEADER_ONLY
#include <jsonrpc/impl/types.ipp>
#endif
