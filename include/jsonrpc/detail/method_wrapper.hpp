#pragma once

#include <jsonrpc/errors.hpp>
#include <jsonrpc/detail/function_traits.hpp>
#include <jsonrpc/detail/type_converter.hpp>
#include <jsonrpc/detail/index_sequence.hpp>
#include <boost/json.hpp>
#include <memory>

/**
 * @file method_wrapper.hpp
 * @brief RPC 方法包装器
 *
 * 提供自动类型转换的方法包装，支持任意函数签名。
 *
 * @author 无事情小神仙
 */

namespace jsonrpc {
namespace detail {

/**
 * @brief 方法包装器基类
 *
 * 提供统一的调用接口，隐藏具体函数类型。
 */
class MethodWrapperBase {
public:
    virtual ~MethodWrapperBase() = default;

    /**
     * @brief 调用方法
     * @param params JSON 参数
     * @return JSON 结果
     * @throws Error 如果参数不匹配或方法执行失败
     */
    virtual boost::json::value invoke(const boost::json::value& params) = 0;
};

// ============================================================================
// 辅助：使用 tuple 调用函数
// ============================================================================

/**
 * @brief 使用 tuple 展开参数调用函数（实现）
 *
 * @tparam Func 函数类型
 * @tparam Tuple tuple 类型
 * @tparam Is 索引序列
 * @param func 函数对象
 * @param tuple 参数 tuple
 * @param 索引序列（编译期）
 * @return 函数返回值
 */
template<typename Func, typename Tuple, size_t... Is>
auto call_with_tuple_impl(Func&& func, Tuple&& tuple, index_sequence<Is...>)
    -> decltype(func(std::get<Is>(tuple)...))
{
    return func(std::get<Is>(std::forward<Tuple>(tuple))...);
}

/**
 * @brief 使用 tuple 展开参数调用函数
 *
 * @tparam Func 函数类型
 * @tparam Tuple tuple 类型
 * @param func 函数对象
 * @param tuple 参数 tuple
 * @return 函数返回值
 */
template<typename Func, typename Tuple>
auto call_with_tuple(Func&& func, Tuple&& tuple)
    -> decltype(call_with_tuple_impl(
        std::forward<Func>(func),
        std::forward<Tuple>(tuple),
        make_index_sequence<std::tuple_size<typename std::decay<Tuple>::type>::value>{}))
{
    return call_with_tuple_impl(
        std::forward<Func>(func),
        std::forward<Tuple>(tuple),
        make_index_sequence<std::tuple_size<typename std::decay<Tuple>::type>::value>{});
}

// ============================================================================
// 方法包装器实现（有返回值）
// ============================================================================

/**
 * @brief 方法包装器实现类（有返回值）
 *
 * @tparam Func 函数类型
 * @tparam R 返回类型
 * @tparam Args 参数类型包
 */
template<typename Func, typename R, typename... Args>
class MethodWrapperImplBase : public MethodWrapperBase {
public:
    explicit MethodWrapperImplBase(Func func)
        : func_(std::move(func))
    {}

    boost::json::value invoke(const boost::json::value& params) override {
        try {
            // 提取参数
            auto args_tuple = extract_args<Args...>(params);

            // 调用函数
            R result = call_with_tuple(func_, std::move(args_tuple));

            // 转换返回值为 JSON
            return json_converter<R>::to_json(result);

        } catch (const Error&) {
            // JSON-RPC 错误直接抛出
            throw;
        } catch (const std::exception& e) {
            // 其他异常转换为 InternalError
            throw Error(ErrorCode::InternalError,
                std::string("方法执行失败: ") + e.what());
        }
    }

private:
    Func func_;
};

// ============================================================================
// 方法包装器实现（无返回值 void）
// ============================================================================

/**
 * @brief 方法包装器实现类（无返回值）
 *
 * @tparam Func 函数类型
 * @tparam Args 参数类型包
 */
template<typename Func, typename... Args>
class MethodWrapperImplBase<Func, void, Args...> : public MethodWrapperBase {
public:
    explicit MethodWrapperImplBase(Func func)
        : func_(std::move(func))
    {}

    boost::json::value invoke(const boost::json::value& params) override {
        try {
            // 提取参数
            auto args_tuple = extract_args<Args...>(params);

            // 调用函数（无返回值）
            call_with_tuple(func_, std::move(args_tuple));

            // 返回 null
            return json_converter<void>::to_json();

        } catch (const Error&) {
            throw;
        } catch (const std::exception& e) {
            throw Error(ErrorCode::InternalError,
                std::string("方法执行失败: ") + e.what());
        }
    }

private:
    Func func_;
};

// ============================================================================
// 方法包装器工厂
// ============================================================================

// 统一的方法包装器实现（自动推导返回类型和参数）
template<typename Func>
class MethodWrapperImpl : public MethodWrapperBase {
public:
    explicit MethodWrapperImpl(Func func)
        : func_(std::move(func))
    {}

    boost::json::value invoke(const boost::json::value& params) override {
        return invoke_impl(params, typename function_traits<Func>::args_tuple{});
    }

private:
    template<typename... Args>
    boost::json::value invoke_impl(const boost::json::value& params, std::tuple<Args...>) {
        typedef typename function_traits<Func>::return_type R;

        try {
            // 提取参数
            auto args_tuple = extract_args<Args...>(params);

            // 调用函数并转换返回值
            return invoke_and_convert<R>(std::move(args_tuple));

        } catch (const Error&) {
            throw;
        } catch (const std::exception& e) {
            throw Error(ErrorCode::InternalError,
                std::string("方法执行失败: ") + e.what());
        }
    }

    // 有返回值的情况
    template<typename R, typename ArgsTuple>
    typename std::enable_if<!std::is_void<R>::value, boost::json::value>::type
    invoke_and_convert(ArgsTuple&& args_tuple) {
        R result = call_with_tuple(func_, std::forward<ArgsTuple>(args_tuple));
        return json_converter<R>::to_json(result);
    }

    // 无返回值的情况
    template<typename R, typename ArgsTuple>
    typename std::enable_if<std::is_void<R>::value, boost::json::value>::type
    invoke_and_convert(ArgsTuple&& args_tuple) {
        call_with_tuple(func_, std::forward<ArgsTuple>(args_tuple));
        return json_converter<void>::to_json();
    }

    Func func_;
};

} // namespace detail
} // namespace jsonrpc
