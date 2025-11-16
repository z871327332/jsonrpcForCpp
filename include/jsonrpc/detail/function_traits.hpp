#pragma once

#include <functional>
#include <tuple>
#include <type_traits>

/**
 * @file function_traits.hpp
 * @brief 函数签名提取工具
 *
 * 提供编译期函数类型分析，支持：
 * - 普通函数指针
 * - std::function
 * - Lambda 表达式
 * - 函数对象
 * - 成员函数指针
 *
 * @author 无事情小神仙
 */

namespace jsonrpc {
namespace detail {

/**
 * @brief 函数特征提取器（主模板）
 *
 * @tparam T 函数类型
 */
template<typename T>
struct function_traits;

// ============================================================================
// 特化：普通函数指针
// ============================================================================

/**
 * @brief 普通函数指针特化
 *
 * @tparam R 返回类型
 * @tparam Args 参数类型包
 */
template<typename R, typename... Args>
struct function_traits<R(*)(Args...)> {
    typedef R return_type;                          ///< 返回类型
    typedef std::tuple<typename std::decay<Args>::type...> args_tuple;         ///< 参数类型元组
    static constexpr size_t arity = sizeof...(Args); ///< 参数个数

    /**
     * @brief 获取第 N 个参数的类型
     *
     * @tparam N 参数索引（从 0 开始）
     */
    template<size_t N>
    struct arg {
        static_assert(N < arity, "参数索引超出范围");
        typedef typename std::tuple_element<N, args_tuple>::type type;
    };
};

// ============================================================================
// 特化：std::function
// ============================================================================

/**
 * @brief std::function 特化
 *
 * @tparam R 返回类型
 * @tparam Args 参数类型包
 */
template<typename R, typename... Args>
struct function_traits<std::function<R(Args...)>> {
    typedef R return_type;
    typedef std::tuple<typename std::decay<Args>::type...> args_tuple;
    static constexpr size_t arity = sizeof...(Args);

    template<size_t N>
    struct arg {
        static_assert(N < arity, "参数索引超出范围");
        typedef typename std::tuple_element<N, args_tuple>::type type;
    };
};

// ============================================================================
// 特化：成员函数指针（const）
// ============================================================================

/**
 * @brief const 成员函数指针特化
 *
 * @tparam C 类类型
 * @tparam R 返回类型
 * @tparam Args 参数类型包
 */
template<typename C, typename R, typename... Args>
struct function_traits<R(C::*)(Args...) const> {
    typedef R return_type;
    typedef std::tuple<typename std::decay<Args>::type...> args_tuple;
    static constexpr size_t arity = sizeof...(Args);

    template<size_t N>
    struct arg {
        static_assert(N < arity, "参数索引超出范围");
        typedef typename std::tuple_element<N, args_tuple>::type type;
    };
};

// ============================================================================
// 特化：成员函数指针（非 const）
// ============================================================================

/**
 * @brief 非 const 成员函数指针特化
 *
 * @tparam C 类类型
 * @tparam R 返回类型
 * @tparam Args 参数类型包
 */
template<typename C, typename R, typename... Args>
struct function_traits<R(C::*)(Args...)> {
    typedef R return_type;
    typedef std::tuple<typename std::decay<Args>::type...> args_tuple;
    static constexpr size_t arity = sizeof...(Args);

    template<size_t N>
    struct arg {
        static_assert(N < arity, "参数索引超出范围");
        typedef typename std::tuple_element<N, args_tuple>::type type;
    };
};

// ============================================================================
// 特化：Lambda 和函数对象（通过 operator() 提取）
// ============================================================================

/**
 * @brief Lambda 和函数对象特化
 *
 * 通过萃取 operator() 成员函数的类型来获取函数签名。
 *
 * @tparam T Lambda 或函数对象类型
 */
template<typename T>
struct function_traits : function_traits<decltype(&T::operator())> {};

// ============================================================================
// 辅助函数：从参数类型包生成 tuple
// ============================================================================

/**
 * @brief 从函数类型提取参数 tuple 类型
 *
 * @tparam Func 函数类型
 */
template<typename Func>
using args_tuple_t = typename function_traits<Func>::args_tuple;

/**
 * @brief 从函数类型提取返回类型
 *
 * @tparam Func 函数类型
 */
template<typename Func>
using return_type_t = typename function_traits<Func>::return_type;

/**
 * @brief 获取函数参数个数
 *
 * @tparam Func 函数类型
 */
} // namespace detail
} // namespace jsonrpc
