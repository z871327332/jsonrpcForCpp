#pragma once

#include <jsonrpc/errors.hpp>
#include <jsonrpc/detail/index_sequence.hpp>
#include <boost/json.hpp>
#include <string>
#include <vector>
#include <map>
#include <type_traits>

/**
 * @file type_converter.hpp
 * @brief JSON 与 C++ 类型相互转换
 *
 * 提供 JSON 值与 C++ 类型之间的自动转换。
 * 支持基础类型、容器类型和自定义类型扩展。
 *
 * @author 无事情小神仙
 */

namespace jsonrpc {
namespace detail {

/**
 * @brief JSON 类型转换器（主模板）
 *
 * @tparam T C++ 类型
 * @tparam Enable SFINAE 启用条件
 */
template<typename T, typename Enable = void>
struct json_converter {
    /**
     * @brief 从 JSON 值转换为 C++ 类型
     * @param jv JSON 值
     * @return C++ 类型的值
     * @throws Error 如果类型不匹配
     */
    static T from_json(const boost::json::value& jv);

    /**
     * @brief 从 C++ 类型转换为 JSON 值
     * @param val C++ 类型的值
     * @return JSON 值
     */
    static boost::json::value to_json(const T& val);
};

// ============================================================================
// 基础类型特化
// ============================================================================

/**
 * @brief int 类型特化
 */
template<>
struct json_converter<int> {
    static int from_json(const boost::json::value& jv) {
        if (!jv.is_int64()) {
            throw Error(ErrorCode::InvalidParams, "期望 int 类型");
        }
        return static_cast<int>(jv.as_int64());
    }

    static boost::json::value to_json(int val) {
        return boost::json::value(val);
    }
};

/**
 * @brief int64_t 类型特化
 */
template<>
struct json_converter<int64_t> {
    static int64_t from_json(const boost::json::value& jv) {
        if (!jv.is_int64()) {
            throw Error(ErrorCode::InvalidParams, "期望 int64 类型");
        }
        return jv.as_int64();
    }

    static boost::json::value to_json(int64_t val) {
        return boost::json::value(val);
    }
};

/**
 * @brief uint64_t 类型特化
 */
template<>
struct json_converter<uint64_t> {
    static uint64_t from_json(const boost::json::value& jv) {
        if (!jv.is_uint64()) {
            throw Error(ErrorCode::InvalidParams, "期望 uint64 类型");
        }
        return jv.as_uint64();
    }

    static boost::json::value to_json(uint64_t val) {
        return boost::json::value(val);
    }
};

/**
 * @brief double 类型特化
 */
template<>
struct json_converter<double> {
    static double from_json(const boost::json::value& jv) {
        if (jv.is_double()) {
            return jv.as_double();
        } else if (jv.is_int64()) {
            return static_cast<double>(jv.as_int64());
        } else if (jv.is_uint64()) {
            return static_cast<double>(jv.as_uint64());
        }
        throw Error(ErrorCode::InvalidParams, "期望 double 类型");
    }

    static boost::json::value to_json(double val) {
        return boost::json::value(val);
    }
};

/**
 * @brief float 类型特化
 */
template<>
struct json_converter<float> {
    static float from_json(const boost::json::value& jv) {
        return static_cast<float>(json_converter<double>::from_json(jv));
    }

    static boost::json::value to_json(float val) {
        return boost::json::value(static_cast<double>(val));
    }
};

/**
 * @brief bool 类型特化
 */
template<>
struct json_converter<bool> {
    static bool from_json(const boost::json::value& jv) {
        if (!jv.is_bool()) {
            throw Error(ErrorCode::InvalidParams, "期望 bool 类型");
        }
        return jv.as_bool();
    }

    static boost::json::value to_json(bool val) {
        return boost::json::value(val);
    }
};

/**
 * @brief std::string 类型特化
 */
template<>
struct json_converter<std::string> {
    static std::string from_json(const boost::json::value& jv) {
        if (!jv.is_string()) {
            throw Error(ErrorCode::InvalidParams, "期望 string 类型");
        }
        return std::string(jv.as_string().c_str());
    }

    static boost::json::value to_json(const std::string& val) {
        return boost::json::value(val);
    }
};

/**
 * @brief const char* 类型特化（仅用于序列化）
 */
template<>
struct json_converter<const char*> {
    static const char* from_json(const boost::json::value& jv) {
        static thread_local std::string storage;
        storage = json_converter<std::string>::from_json(jv);
        return storage.c_str();
    }

    static boost::json::value to_json(const char* val) {
        if (!val) {
            return boost::json::value("");
        }
        return json_converter<std::string>::to_json(std::string(val));
    }
};

// ============================================================================
// 容器类型特化
// ============================================================================

/**
 * @brief std::vector<T> 类型特化
 *
 * @tparam T 元素类型
 */
template<typename T>
struct json_converter<std::vector<T>> {
    static std::vector<T> from_json(const boost::json::value& jv) {
        if (!jv.is_array()) {
            throw Error(ErrorCode::InvalidParams, "期望 array 类型");
        }

        const auto& arr = jv.as_array();
        std::vector<T> result;
        result.reserve(arr.size());

        for (const auto& elem : arr) {
            result.push_back(json_converter<T>::from_json(elem));
        }

        return result;
    }

    static boost::json::value to_json(const std::vector<T>& val) {
        boost::json::array arr;
        for (const auto& elem : val) {
            arr.push_back(json_converter<T>::to_json(elem));
        }
        return arr;
    }
};

/**
 * @brief std::map<std::string, T> 类型特化
 *
 * @tparam T 值类型
 */
template<typename T>
struct json_converter<std::map<std::string, T>> {
    static std::map<std::string, T> from_json(const boost::json::value& jv) {
        if (!jv.is_object()) {
            throw Error(ErrorCode::InvalidParams, "期望 object 类型");
        }

        const auto& obj = jv.as_object();
        std::map<std::string, T> result;

        for (const auto& pair : obj) {
            auto key_view = pair.key();
            std::string key(key_view.data(), key_view.size());
            result[key] = json_converter<T>::from_json(pair.value());
        }

        return result;
    }

    static boost::json::value to_json(const std::map<std::string, T>& val) {
        boost::json::object obj;
        for (const auto& pair : val) {
            obj[pair.first] = json_converter<T>::to_json(pair.second);
        }
        return obj;
    }
};

// ============================================================================
// void 类型特化（用于无返回值的函数）
// ============================================================================

/**
 * @brief void 类型特化
 *
 * 用于处理无返回值的函数。
 */
template<>
struct json_converter<void> {
    static boost::json::value to_json() {
        return boost::json::value(nullptr);
    }
};

// ============================================================================
// 参数提取辅助函数
// ============================================================================

/**
 * @brief 从 JSON array 提取参数到 tuple（实现）
 *
 * @tparam Tuple tuple 类型
 * @tparam Is 索引序列
 * @param arr JSON array
 * @param 索引序列（编译期）
 * @return 提取的 tuple
 * @throws Error 如果参数数量不匹配或类型不匹配
 */
template<typename Tuple, size_t... Is>
Tuple extract_args_impl(const boost::json::array& arr, index_sequence<Is...>) {
    constexpr size_t N = sizeof...(Is);

    if (arr.size() != N) {
        throw Error(ErrorCode::InvalidParams,
            "参数数量不匹配：期望 " + std::to_string(N) +
            " 个，实际 " + std::to_string(arr.size()) + " 个");
    }

    return std::make_tuple(
        json_converter<typename std::tuple_element<Is, Tuple>::type>::from_json(arr[Is])...
    );
}

/**
 * @brief 从 JSON params 提取参数到 tuple
 *
 * @tparam Args 参数类型包
 * @param params JSON params（应为 array）
 * @return 提取的参数 tuple
 * @throws Error 如果 params 不是 array 或参数不匹配
 */
template<typename... Args>
std::tuple<Args...> extract_args(const boost::json::value& params) {
    if (!params.is_array()) {
        throw Error(ErrorCode::InvalidParams, "params 必须是 array");
    }

    return extract_args_impl<std::tuple<Args...>>(
        params.as_array(),
        make_index_sequence<sizeof...(Args)>{}
    );
}

/**
 * @brief 特化：无参数情况
 */
template<>
inline std::tuple<> extract_args<>(const boost::json::value& params) {
    // 无参数时，params 可以是 null 或空 array
    if (!params.is_null() && !params.is_array()) {
        throw Error(ErrorCode::InvalidParams, "params 必须是 null 或 array");
    }

    if (params.is_array() && !params.as_array().empty()) {
        throw Error(ErrorCode::InvalidParams, "期望无参数，但提供了参数");
    }

    return std::tuple<>();
}

} // namespace detail
} // namespace jsonrpc
