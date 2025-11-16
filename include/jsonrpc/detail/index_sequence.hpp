#pragma once

/**
 * @file index_sequence.hpp
 * @brief C++11 版本的 index_sequence 实现
 *
 * C++14 引入了 std::index_sequence，这里提供 C++11 兼容的实现。
 *
 * @author 无事情小神仙
 */

namespace jsonrpc {
namespace detail {

/**
 * @brief 整数序列（C++11 版本）
 *
 * 用于模板元编程中的索引展开。
 *
 * @tparam Is 整数序列
 */
template<size_t... Is>
struct index_sequence {
    typedef size_t value_type;
    static constexpr size_t size() noexcept { return sizeof...(Is); }
};

/**
 * @brief 生成 index_sequence 的辅助结构（实现细节）
 *
 * @tparam N 当前索引
 * @tparam Is 已累积的索引序列
 */
template<size_t N, size_t... Is>
struct make_index_sequence_impl : make_index_sequence_impl<N - 1, N - 1, Is...> {};

/**
 * @brief 递归终止特化
 *
 * @tparam Is 完整的索引序列
 */
template<size_t... Is>
struct make_index_sequence_impl<0, Is...> {
    typedef index_sequence<Is...> type;
};

/**
 * @brief 生成 0 到 N-1 的 index_sequence
 *
 * @tparam N 序列长度
 *
 * 用法示例：
 * @code
 * make_index_sequence<3>  // 生成 index_sequence<0, 1, 2>
 * @endcode
 */
template<size_t N>
using make_index_sequence = typename make_index_sequence_impl<N>::type;

/**
 * @brief 从类型包生成 index_sequence
 *
 * @tparam T 类型包
 *
 * 用法示例：
 * @code
 * index_sequence_for<int, double, std::string>  // 生成 index_sequence<0, 1, 2>
 * @endcode
 */
template<typename... T>
using index_sequence_for = make_index_sequence<sizeof...(T)>;

} // namespace detail
} // namespace jsonrpc
