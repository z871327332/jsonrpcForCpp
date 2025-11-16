#include <jsonrpc/jsonrpc.hpp>
#include <jsonrpc/detail/type_converter.hpp>
#include <iostream>
#include <string>

struct User {
    std::string name;
    int age;
};

namespace jsonrpc {
namespace detail {

template<>
struct json_converter<User> {
    static User from_json(const boost::json::value& jv) {
        if (!jv.is_object()) {
            throw Error(ErrorCode::InvalidParams, "期望 object 类型");
        }

        const auto& obj = jv.as_object();
        User user;
        user.name = std::string(obj.at("name").as_string().c_str());
        user.age = static_cast<int>(obj.at("age").as_int64());
        return user;
    }

    static boost::json::value to_json(const User& user) {
        boost::json::object obj;
        obj["name"] = user.name;
        obj["age"] = user.age;
        return obj;
    }
};

} // namespace detail
} // namespace jsonrpc

int main() {
    User user{"Alice", 28};
    auto json_value = jsonrpc::detail::json_converter<User>::to_json(user);
    std::cout << "序列化结果: " << boost::json::serialize(json_value) << std::endl;

    auto parsed_user = jsonrpc::detail::json_converter<User>::from_json(json_value);
    std::cout << "反序列化: " << parsed_user.name << ", age = " << parsed_user.age << std::endl;

    return 0;
}
