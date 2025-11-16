#include <jsonrpc/detail/type_converter.hpp>
#include <gtest/gtest.h>
#include <limits>

using namespace jsonrpc::detail;

// ============================================================================
// 现有测试（保留）
// ============================================================================

TEST(TypeConverterTest, VectorConversion) {
    std::vector<int> numbers = {1, 2, 3};
    auto json = json_converter<std::vector<int>>::to_json(numbers);
    auto parsed = json_converter<std::vector<int>>::from_json(json);
    ASSERT_EQ(parsed.size(), numbers.size());
    EXPECT_EQ(parsed[0], 1);
    EXPECT_EQ(parsed[2], 3);
}

TEST(TypeConverterTest, MapConversion) {
    std::map<std::string, int> data{{"a", 1}, {"b", 2}};
    auto json = json_converter<std::map<std::string, int>>::to_json(data);
    auto parsed = json_converter<std::map<std::string, int>>::from_json(json);
    EXPECT_EQ(parsed.at("a"), 1);
    EXPECT_EQ(parsed.at("b"), 2);
}

// ============================================================================
// 分组 1：基本类型转换（8 个，编号 3-10）
// ============================================================================

TEST(TypeConverterTest, ConvertInt) {
    int value = 42;
    auto json = json_converter<int>::to_json(value);
    EXPECT_TRUE(json.is_int64());
    EXPECT_EQ(json.as_int64(), 42);

    int parsed = json_converter<int>::from_json(json);
    EXPECT_EQ(parsed, 42);
}

TEST(TypeConverterTest, ConvertLong) {
    long value = 123456789L;
    auto json = json_converter<long>::to_json(value);
    EXPECT_TRUE(json.is_int64());

    long parsed = json_converter<long>::from_json(json);
    EXPECT_EQ(parsed, 123456789L);
}

TEST(TypeConverterTest, ConvertInt64) {
    int64_t value = 9876543210LL;
    auto json = json_converter<int64_t>::to_json(value);
    EXPECT_TRUE(json.is_int64());

    int64_t parsed = json_converter<int64_t>::from_json(json);
    EXPECT_EQ(parsed, 9876543210LL);
}

TEST(TypeConverterTest, ConvertDouble) {
    double value = 3.14159;
    auto json = json_converter<double>::to_json(value);
    EXPECT_TRUE(json.is_double());

    double parsed = json_converter<double>::from_json(json);
    EXPECT_NEAR(parsed, 3.14159, 0.00001);
}

TEST(TypeConverterTest, ConvertFloat) {
    float value = 2.71828f;
    auto json = json_converter<float>::to_json(value);
    EXPECT_TRUE(json.is_double());

    float parsed = json_converter<float>::from_json(json);
    EXPECT_NEAR(parsed, 2.71828f, 0.0001f);
}

TEST(TypeConverterTest, ConvertBool) {
    // 测试 true
    bool value_true = true;
    auto json_true = json_converter<bool>::to_json(value_true);
    EXPECT_TRUE(json_true.is_bool());
    EXPECT_TRUE(json_true.as_bool());

    bool parsed_true = json_converter<bool>::from_json(json_true);
    EXPECT_TRUE(parsed_true);

    // 测试 false
    bool value_false = false;
    auto json_false = json_converter<bool>::to_json(value_false);
    EXPECT_TRUE(json_false.is_bool());
    EXPECT_FALSE(json_false.as_bool());

    bool parsed_false = json_converter<bool>::from_json(json_false);
    EXPECT_FALSE(parsed_false);
}

TEST(TypeConverterTest, ConvertString) {
    std::string value = "Hello, JsonRPC!";
    auto json = json_converter<std::string>::to_json(value);
    EXPECT_TRUE(json.is_string());
    EXPECT_EQ(json.as_string(), "Hello, JsonRPC!");

    std::string parsed = json_converter<std::string>::from_json(json);
    EXPECT_EQ(parsed, "Hello, JsonRPC!");
}

TEST(TypeConverterTest, ConvertVoid) {
    // void 类型转换为 null
    auto json = json_converter<void>::to_json();
    EXPECT_TRUE(json.is_null());
}

// ============================================================================
// 分组 2：容器类型转换（补充 4 个，编号 11-14）
// ============================================================================

TEST(TypeConverterTest, ConvertVectorString) {
    std::vector<std::string> values = {"apple", "banana", "cherry"};
    auto json = json_converter<std::vector<std::string>>::to_json(values);
    EXPECT_TRUE(json.is_array());
    EXPECT_EQ(json.as_array().size(), 3u);

    auto parsed = json_converter<std::vector<std::string>>::from_json(json);
    ASSERT_EQ(parsed.size(), 3u);
    EXPECT_EQ(parsed[0], "apple");
    EXPECT_EQ(parsed[1], "banana");
    EXPECT_EQ(parsed[2], "cherry");
}

TEST(TypeConverterTest, ConvertVectorDouble) {
    std::vector<double> values = {1.1, 2.2, 3.3, 4.4};
    auto json = json_converter<std::vector<double>>::to_json(values);

    auto parsed = json_converter<std::vector<double>>::from_json(json);
    ASSERT_EQ(parsed.size(), 4u);
    EXPECT_NEAR(parsed[0], 1.1, 0.01);
    EXPECT_NEAR(parsed[3], 4.4, 0.01);
}

TEST(TypeConverterTest, ConvertMapStringString) {
    std::map<std::string, std::string> data;
    data["name"] = "Alice";
    data["city"] = "Beijing";

    auto json = json_converter<std::map<std::string, std::string>>::to_json(data);
    EXPECT_TRUE(json.is_object());

    auto parsed = json_converter<std::map<std::string, std::string>>::from_json(json);
    EXPECT_EQ(parsed.at("name"), "Alice");
    EXPECT_EQ(parsed.at("city"), "Beijing");
}

TEST(TypeConverterTest, ConvertEmptyVector) {
    std::vector<int> empty_vec;
    auto json = json_converter<std::vector<int>>::to_json(empty_vec);
    EXPECT_TRUE(json.is_array());
    EXPECT_EQ(json.as_array().size(), 0u);

    auto parsed = json_converter<std::vector<int>>::from_json(json);
    EXPECT_EQ(parsed.size(), 0u);
}

// ============================================================================
// 分组 3：嵌套类型转换（6 个，编号 15-20）
// ============================================================================

TEST(TypeConverterTest, ConvertVectorVector) {
    std::vector<std::vector<int>> matrix = {
        {1, 2, 3},
        {4, 5, 6},
        {7, 8, 9}
    };

    auto json = json_converter<std::vector<std::vector<int>>>::to_json(matrix);
    EXPECT_TRUE(json.is_array());

    auto parsed = json_converter<std::vector<std::vector<int>>>::from_json(json);
    ASSERT_EQ(parsed.size(), 3u);
    ASSERT_EQ(parsed[0].size(), 3u);
    EXPECT_EQ(parsed[0][0], 1);
    EXPECT_EQ(parsed[1][1], 5);
    EXPECT_EQ(parsed[2][2], 9);
}

TEST(TypeConverterTest, ConvertMapVector) {
    std::map<std::string, std::vector<int>> data;
    data["scores"] = {90, 85, 92};
    data["ages"] = {25, 30, 28};

    auto json = json_converter<std::map<std::string, std::vector<int>>>::to_json(data);

    auto parsed = json_converter<std::map<std::string, std::vector<int>>>::from_json(json);
    ASSERT_EQ(parsed.at("scores").size(), 3u);
    EXPECT_EQ(parsed.at("scores")[0], 90);
    EXPECT_EQ(parsed.at("ages")[1], 30);
}

TEST(TypeConverterTest, ConvertVectorMap) {
    std::vector<std::map<std::string, int>> users;

    std::map<std::string, int> user1;
    user1["id"] = 1;
    user1["age"] = 25;
    users.push_back(user1);

    std::map<std::string, int> user2;
    user2["id"] = 2;
    user2["age"] = 30;
    users.push_back(user2);

    auto json = json_converter<std::vector<std::map<std::string, int>>>::to_json(users);

    auto parsed = json_converter<std::vector<std::map<std::string, int>>>::from_json(json);
    ASSERT_EQ(parsed.size(), 2u);
    EXPECT_EQ(parsed[0].at("id"), 1);
    EXPECT_EQ(parsed[1].at("age"), 30);
}

TEST(TypeConverterTest, ConvertNestedMap) {
    std::map<std::string, std::map<std::string, int>> data;
    data["user1"]["score"] = 95;
    data["user1"]["level"] = 5;
    data["user2"]["score"] = 88;
    data["user2"]["level"] = 3;

    auto json = json_converter<std::map<std::string, std::map<std::string, int>>>::to_json(data);

    auto parsed = json_converter<std::map<std::string, std::map<std::string, int>>>::from_json(json);
    EXPECT_EQ(parsed.at("user1").at("score"), 95);
    EXPECT_EQ(parsed.at("user2").at("level"), 3);
}

TEST(TypeConverterTest, ConvertComplexNested) {
    // 三层嵌套：vector<map<string, vector<int>>>
    std::vector<std::map<std::string, std::vector<int>>> complex;

    std::map<std::string, std::vector<int>> item;
    item["numbers"] = {1, 2, 3};
    item["values"] = {10, 20, 30};
    complex.push_back(item);

    auto json = json_converter<std::vector<std::map<std::string, std::vector<int>>>>::to_json(complex);

    auto parsed = json_converter<std::vector<std::map<std::string, std::vector<int>>>>::from_json(json);
    ASSERT_EQ(parsed.size(), 1u);
    ASSERT_EQ(parsed[0].at("numbers").size(), 3u);
    EXPECT_EQ(parsed[0].at("numbers")[1], 2);
    EXPECT_EQ(parsed[0].at("values")[2], 30);
}

TEST(TypeConverterTest, ConvertEmptyNested) {
    std::vector<std::vector<int>> empty_matrix;
    auto json = json_converter<std::vector<std::vector<int>>>::to_json(empty_matrix);

    auto parsed = json_converter<std::vector<std::vector<int>>>::from_json(json);
    EXPECT_EQ(parsed.size(), 0u);

    // 嵌套空 vector
    std::vector<std::vector<int>> matrix_with_empty = {{}};
    auto json2 = json_converter<std::vector<std::vector<int>>>::to_json(matrix_with_empty);
    auto parsed2 = json_converter<std::vector<std::vector<int>>>::from_json(json2);
    ASSERT_EQ(parsed2.size(), 1u);
    EXPECT_EQ(parsed2[0].size(), 0u);
}

// ============================================================================
// 分组 4：错误处理（5 个，编号 21-25）
// ============================================================================

TEST(TypeConverterTest, ConvertInvalidInt) {
    // 尝试从字符串转换为 int 应该抛出异常
    boost::json::value json_str = "not_a_number";

    EXPECT_THROW(json_converter<int>::from_json(json_str), std::exception);
}

TEST(TypeConverterTest, ConvertInvalidBool) {
    // 尝试从数字转换为 bool 应该抛出异常
    boost::json::value json_num = 42;

    EXPECT_THROW(json_converter<bool>::from_json(json_num), std::exception);
}

TEST(TypeConverterTest, ConvertInvalidVector) {
    // 尝试从对象转换为 vector 应该抛出异常
    boost::json::value json_obj = boost::json::object{{"key", "value"}};

    EXPECT_THROW(json_converter<std::vector<int>>::from_json(json_obj), std::exception);
}

TEST(TypeConverterTest, ConvertInvalidMap) {
    // 尝试从数组转换为 map 应该抛出异常
    boost::json::value json_arr = boost::json::array{1, 2, 3};

    EXPECT_THROW((json_converter<std::map<std::string, int>>::from_json(json_arr)), std::exception);
}

TEST(TypeConverterTest, ConvertNullValue) {
    // null 值转换测试
    boost::json::value json_null = nullptr;

    // 整数类型遇到 null 应该抛出异常
    EXPECT_THROW(json_converter<int>::from_json(json_null), std::exception);

    // 字符串类型遇到 null 应该抛出异常
    EXPECT_THROW(json_converter<std::string>::from_json(json_null), std::exception);
}

// ============================================================================
// 分组 5：边界情况（2 个，编号 26-27）
// ============================================================================

TEST(TypeConverterTest, ConvertLargeVector) {
    // 大型 vector（1000 个元素）
    std::vector<int> large_vec;
    for (int i = 0; i < 1000; i++) {
        large_vec.push_back(i);
    }

    auto json = json_converter<std::vector<int>>::to_json(large_vec);
    EXPECT_TRUE(json.is_array());
    EXPECT_EQ(json.as_array().size(), 1000u);

    auto parsed = json_converter<std::vector<int>>::from_json(json);
    ASSERT_EQ(parsed.size(), 1000u);
    EXPECT_EQ(parsed[0], 0);
    EXPECT_EQ(parsed[500], 500);
    EXPECT_EQ(parsed[999], 999);
}

TEST(TypeConverterTest, ConvertLargeMap) {
    // 大型 map（100 个键值对）
    std::map<std::string, int> large_map;
    for (int i = 0; i < 100; i++) {
        large_map["key_" + std::to_string(i)] = i * 10;
    }

    auto json = json_converter<std::map<std::string, int>>::to_json(large_map);
    EXPECT_TRUE(json.is_object());

    auto parsed = json_converter<std::map<std::string, int>>::from_json(json);
    EXPECT_EQ(parsed.size(), 100u);
    EXPECT_EQ(parsed.at("key_0"), 0);
    EXPECT_EQ(parsed.at("key_50"), 500);
    EXPECT_EQ(parsed.at("key_99"), 990);
}
