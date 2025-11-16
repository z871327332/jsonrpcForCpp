#pragma once

/**
 * @file jsonrpc.hpp
 * @brief JsonRPC C++ Library - 主包含文件
 *
 * 这是 JsonRPC 库的单一入口点，包含所有公共 API。
 * 用户只需包含此文件即可使用完整功能。
 *
 * @author 无事情小神仙
 * @version 1.0.0
 */

#include <jsonrpc/config.hpp>
#include <jsonrpc/errors.hpp>
#include <jsonrpc/types.hpp>
#include <jsonrpc/server.hpp>
#include <jsonrpc/client.hpp>

/**
 * @namespace jsonrpc
 * @brief JsonRPC 库的主命名空间
 *
 * 所有公共 API 都在此命名空间中。
 */
namespace jsonrpc {

/**
 * @namespace jsonrpc::detail
 * @brief 内部实现细节
 *
 * 此命名空间包含库的内部实现，用户不应直接使用。
 */
namespace detail {}

} // namespace jsonrpc
