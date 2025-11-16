#pragma once

#include <jsonrpc/types.hpp>
#include <jsonrpc/errors.hpp>
#include <boost/json.hpp>
#include <string>
#include <vector>

/**
 * @file protocol.hpp
 * @brief JSON-RPC 2.0 协议处理
 *
 * 提供 JSON-RPC 2.0 协议的解析、序列化和验证功能。
 *
 * @author 无事情小神仙
 */

namespace jsonrpc {
namespace detail {

/**
 * @brief JSON-RPC 2.0 协议处理器
 *
 * 提供静态方法用于处理 JSON-RPC 2.0 协议的各个方面。
 */
class Protocol {
public:
    /**
     * @brief 解析 JSON-RPC 请求
     *
     * 支持单个请求和批量请求（JSON array）。
     *
     * @param json_str JSON 字符串
     * @return 请求对象列表（单个请求返回包含 1 个元素的 vector）
     * @throws Error 如果解析失败或请求无效
     */
    static std::vector<Request> parse_request(const std::string& json_str);

    /**
     * @brief 序列化单个响应
     *
     * @param response 响应对象
     * @return JSON 字符串
     */
    static std::string serialize_response(const Response& response);

    /**
     * @brief 序列化批量响应
     *
     * @param responses 响应对象列表
     * @return JSON 字符串（JSON array）
     */
    static std::string serialize_batch_response(const std::vector<Response>& responses);

    /**
     * @brief 验证 JSON-RPC 版本字段
     *
     * @param obj JSON 对象
     * @return 如果版本字段有效返回 true
     */
    static bool validate_version(const boost::json::object& obj);

    /**
     * @brief 判断是否为批量请求
     *
     * @param jv JSON 值
     * @return 如果是批量请求（JSON array）返回 true
     */
    static bool is_batch_request(const boost::json::value& jv);

    /**
     * @brief 序列化单个请求（客户端用）
     *
     * @param request 请求对象
     * @return JSON 字符串
     */
    static std::string serialize_request(const Request& request);

    /**
     * @brief 序列化批量请求（客户端用）
     *
     * @param requests 请求对象列表
     * @return JSON 字符串（JSON array）
     */
    static std::string serialize_batch_request(const std::vector<Request>& requests);

    /**
     * @brief 解析单个响应（客户端用）
     *
     * @param json_str JSON 字符串
     * @return 响应对象
     * @throws Error 如果解析失败或响应无效
     */
    static Response parse_response(const std::string& json_str);

    /**
     * @brief 解析批量响应（客户端用）
     *
     * @param json_str JSON 字符串（JSON array）
     * @return 响应对象列表
     * @throws Error 如果解析失败或响应无效
     */
    static std::vector<Response> parse_batch_response(const std::string& json_str);
};

} // namespace detail
} // namespace jsonrpc

// Header-only 模式下包含实现
#ifdef JSONRPC_HEADER_ONLY
#include <jsonrpc/impl/protocol.ipp>
#endif
