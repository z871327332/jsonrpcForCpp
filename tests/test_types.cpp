#include <jsonrpc/types.hpp>
#include <gtest/gtest.h>

using namespace jsonrpc;

// ============================================================================
// Request 测试
// ============================================================================

TEST(RequestTest, ConstructorWithId) {
    boost::json::array params = {1, 2, 3};
    boost::json::value id = 123;

    Request req("add", params, id);

    EXPECT_EQ(req.method(), "add");
    EXPECT_TRUE(req.params().is_array());
    EXPECT_TRUE(req.has_id());
    EXPECT_EQ(req.id().as_int64(), 123);
}

TEST(RequestTest, ConstructorWithoutId) {
    boost::json::array params = {1, 2};

    Request req("notify", params);

    EXPECT_EQ(req.method(), "notify");
    EXPECT_TRUE(req.params().is_array());
    EXPECT_FALSE(req.has_id());
}

TEST(RequestTest, ToJson) {
    boost::json::array params = {10, 20};
    boost::json::value id = "req-1";

    Request req("multiply", params, id);
    boost::json::object obj = req.to_json();

    EXPECT_EQ(obj.at("jsonrpc").as_string(), "2.0");
    EXPECT_EQ(obj.at("method").as_string(), "multiply");
    EXPECT_TRUE(obj.at("params").is_array());
    EXPECT_EQ(obj.at("id").as_string(), "req-1");
}

TEST(RequestTest, FromJson) {
    boost::json::object obj = {
        {"jsonrpc", "2.0"},
        {"method", "subtract"},
        {"params", boost::json::array{5, 3}},
        {"id", 42}
    };

    Request req = Request::from_json(obj);

    EXPECT_EQ(req.method(), "subtract");
    EXPECT_TRUE(req.params().is_array());
    EXPECT_EQ(req.params().as_array().size(), 2u);
    EXPECT_TRUE(req.has_id());
    EXPECT_EQ(req.id().as_int64(), 42);
}

TEST(RequestTest, FromJsonNotification) {
    boost::json::object obj = {
        {"jsonrpc", "2.0"},
        {"method", "update"},
        {"params", boost::json::array{1, 2, 3, 4, 5}}
    };

    Request req = Request::from_json(obj);

    EXPECT_EQ(req.method(), "update");
    EXPECT_FALSE(req.has_id());
}

// ============================================================================
// Response 测试
// ============================================================================

TEST(ResponseTest, ConstructorWithResult) {
    boost::json::value result = 42;
    boost::json::value id = 1;

    Response resp(result, id);

    EXPECT_FALSE(resp.is_error());
    EXPECT_EQ(resp.result().as_int64(), 42);
    EXPECT_EQ(resp.id().as_int64(), 1);
}

TEST(ResponseTest, ConstructorWithError) {
    Error error(ErrorCode::MethodNotFound, "方法不存在");
    boost::json::value id = "req-2";

    Response resp(error, id);

    EXPECT_TRUE(resp.is_error());
    EXPECT_EQ(resp.error().code(), ErrorCode::MethodNotFound);
    EXPECT_EQ(resp.error().message(), "方法不存在");
    EXPECT_EQ(resp.id().as_string(), "req-2");
}

TEST(ResponseTest, ToJsonWithResult) {
    boost::json::value result = boost::json::object{{"sum", 100}};
    boost::json::value id = 5;

    Response resp(result, id);
    boost::json::object obj = resp.to_json();

    EXPECT_EQ(obj.at("jsonrpc").as_string(), "2.0");
    EXPECT_TRUE(obj.contains("result"));
    EXPECT_EQ(obj.at("result").as_object().at("sum").as_int64(), 100);
    EXPECT_EQ(obj.at("id").as_int64(), 5);
    EXPECT_FALSE(obj.contains("error"));
}

TEST(ResponseTest, ToJsonWithError) {
    Error error(ErrorCode::InvalidParams, "参数无效");
    boost::json::value id = boost::json::value(nullptr);

    Response resp(error, id);
    boost::json::object obj = resp.to_json();

    EXPECT_EQ(obj.at("jsonrpc").as_string(), "2.0");
    EXPECT_TRUE(obj.contains("error"));
    EXPECT_EQ(obj.at("error").as_object().at("code").as_int64(),
              static_cast<int>(ErrorCode::InvalidParams));
    EXPECT_EQ(obj.at("error").as_object().at("message").as_string(), "参数无效");
    EXPECT_TRUE(obj.at("id").is_null());
    EXPECT_FALSE(obj.contains("result"));
}

TEST(ResponseTest, FromJsonWithResult) {
    boost::json::object obj = {
        {"jsonrpc", "2.0"},
        {"result", "success"},
        {"id", 10}
    };

    Response resp = Response::from_json(obj);

    EXPECT_FALSE(resp.is_error());
    EXPECT_EQ(resp.result().as_string(), "success");
    EXPECT_EQ(resp.id().as_int64(), 10);
}

TEST(ResponseTest, FromJsonWithError) {
    boost::json::object obj = {
        {"jsonrpc", "2.0"},
        {"error", boost::json::object{
            {"code", -32601},
            {"message", "Method not found"}
        }},
        {"id", boost::json::value(nullptr)}
    };

    Response resp = Response::from_json(obj);

    EXPECT_TRUE(resp.is_error());
    EXPECT_EQ(resp.error().code(), ErrorCode::MethodNotFound);
    EXPECT_EQ(resp.error().message(), "Method not found");
    EXPECT_TRUE(resp.id().is_null());
}

// ============================================================================
// Error 测试
// ============================================================================

TEST(ErrorTest, Constructor) {
    Error err(ErrorCode::ParseError, "解析失败");

    EXPECT_EQ(err.code(), ErrorCode::ParseError);
    EXPECT_EQ(err.message(), "解析失败");
}

TEST(ErrorTest, WhatMethod) {
    Error err(ErrorCode::InternalError, "内部错误");

    std::string what_msg = err.what();
    EXPECT_TRUE(what_msg.find("内部错误") != std::string::npos);
}
