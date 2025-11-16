#include <jsonrpc/detail/protocol.hpp>
#include <gtest/gtest.h>

using namespace jsonrpc::detail;
using namespace jsonrpc;

// ============================================================================
// 现有测试（保留）
// ============================================================================

TEST(ProtocolTest, ParseSingleRequest) {
    std::string payload = R"({"jsonrpc":"2.0","method":"ping","id":1})";
    auto requests = Protocol::parse_request(payload);
    ASSERT_EQ(requests.size(), 1u);
    EXPECT_EQ(requests[0].method(), "ping");
    EXPECT_TRUE(requests[0].has_id());
}

TEST(ProtocolTest, SerializeResponse) {
    jsonrpc::Response response(boost::json::value(42), boost::json::value(1));
    auto json = Protocol::serialize_response(response);
    EXPECT_NE(json.find("\"result\":42"), std::string::npos);
    EXPECT_NE(json.find("\"id\":1"), std::string::npos);
}

TEST(ProtocolTest, ParseInvalidRequestThrows) {
    std::string invalid_payload = R"({"jsonrpc":"1.0"})";
    EXPECT_THROW(Protocol::parse_request(invalid_payload), jsonrpc::Error);
}

// ============================================================================
// 分组 1：批量请求解析/序列化（6 个，编号 4-9）
// ============================================================================

TEST(ProtocolTest, ParseBatchRequest) {
    std::string payload = R"([
        {"jsonrpc":"2.0","method":"add","params":[1,2],"id":1},
        {"jsonrpc":"2.0","method":"subtract","params":[5,3],"id":2}
    ])";

    auto requests = Protocol::parse_request(payload);
    ASSERT_EQ(requests.size(), 2u);

    EXPECT_EQ(requests[0].method(), "add");
    EXPECT_TRUE(requests[0].has_id());
    EXPECT_EQ(requests[0].id().as_int64(), 1);

    EXPECT_EQ(requests[1].method(), "subtract");
    EXPECT_TRUE(requests[1].has_id());
    EXPECT_EQ(requests[1].id().as_int64(), 2);
}

TEST(ProtocolTest, ParseEmptyBatchRequest) {
    std::string payload = R"([])";

    // 空批量请求应该抛出 InvalidRequest 错误
    EXPECT_THROW({
        Protocol::parse_request(payload);
    }, jsonrpc::Error);
}

TEST(ProtocolTest, ParseMixedBatchRequest) {
    std::string payload = R"([
        {"jsonrpc":"2.0","method":"add","params":[1,2],"id":1},
        {"jsonrpc":"2.0","method":"notify","params":["hello"]},
        {"jsonrpc":"2.0","method":"multiply","params":[3,4],"id":2}
    ])";

    auto requests = Protocol::parse_request(payload);
    ASSERT_EQ(requests.size(), 3u);

    // 第一个是请求
    EXPECT_TRUE(requests[0].has_id());
    EXPECT_EQ(requests[0].method(), "add");

    // 第二个是通知（无 ID）
    EXPECT_FALSE(requests[1].has_id());
    EXPECT_EQ(requests[1].method(), "notify");

    // 第三个是请求
    EXPECT_TRUE(requests[2].has_id());
    EXPECT_EQ(requests[2].method(), "multiply");
}

TEST(ProtocolTest, ParseNotificationRequest) {
    std::string payload = R"({"jsonrpc":"2.0","method":"update","params":[1,2,3]})";

    auto requests = Protocol::parse_request(payload);
    ASSERT_EQ(requests.size(), 1u);

    EXPECT_EQ(requests[0].method(), "update");
    EXPECT_FALSE(requests[0].has_id());
}

TEST(ProtocolTest, SerializeBatchResponse) {
    std::vector<Response> responses;
    responses.push_back(Response(boost::json::value(3), boost::json::value(1)));
    responses.push_back(Response(boost::json::value(2), boost::json::value(2)));

    std::string json = Protocol::serialize_batch_response(responses);

    // 应该是 JSON 数组
    EXPECT_EQ(json[0], '[');
    EXPECT_EQ(json[json.length() - 1], ']');

    // 应该包含两个响应
    EXPECT_NE(json.find("\"id\":1"), std::string::npos);
    EXPECT_NE(json.find("\"id\":2"), std::string::npos);
    EXPECT_NE(json.find("\"result\":3"), std::string::npos);
    EXPECT_NE(json.find("\"result\":2"), std::string::npos);
}

TEST(ProtocolTest, SerializeEmptyBatchResponse) {
    std::vector<Response> empty_responses;
    std::string json = Protocol::serialize_batch_response(empty_responses);

    // 空批量响应应该是空数组
    EXPECT_EQ(json, "[]");
}

// ============================================================================
// 分组 2：响应解析（客户端侧，4 个，编号 10-13）
// ============================================================================

TEST(ProtocolTest, ParseResponseWithResult) {
    std::string payload = R"({"jsonrpc":"2.0","result":42,"id":1})";

    Response resp = Protocol::parse_response(payload);

    EXPECT_FALSE(resp.is_error());
    EXPECT_EQ(resp.result().as_int64(), 42);
    EXPECT_EQ(resp.id().as_int64(), 1);
}

TEST(ProtocolTest, ParseResponseWithError) {
    std::string payload = R"({
        "jsonrpc":"2.0",
        "error":{"code":-32601,"message":"Method not found"},
        "id":1
    })";

    Response resp = Protocol::parse_response(payload);

    EXPECT_TRUE(resp.is_error());
    EXPECT_EQ(resp.error().code(), ErrorCode::MethodNotFound);
    EXPECT_EQ(resp.error().message(), "Method not found");
    EXPECT_EQ(resp.id().as_int64(), 1);
}

TEST(ProtocolTest, ParseBatchResponse) {
    std::string payload = R"([
        {"jsonrpc":"2.0","result":3,"id":1},
        {"jsonrpc":"2.0","result":2,"id":2},
        {"jsonrpc":"2.0","error":{"code":-32601,"message":"Not found"},"id":3}
    ])";

    std::vector<Response> responses = Protocol::parse_batch_response(payload);

    ASSERT_EQ(responses.size(), 3u);

    // 第一个响应
    EXPECT_FALSE(responses[0].is_error());
    EXPECT_EQ(responses[0].result().as_int64(), 3);

    // 第二个响应
    EXPECT_FALSE(responses[1].is_error());
    EXPECT_EQ(responses[1].result().as_int64(), 2);

    // 第三个响应是错误
    EXPECT_TRUE(responses[2].is_error());
    EXPECT_EQ(responses[2].error().code(), ErrorCode::MethodNotFound);
}

TEST(ProtocolTest, SerializeErrorResponse) {
    Error error(ErrorCode::InvalidParams, "参数无效");
    Response response(error, boost::json::value(1));

    std::string json = Protocol::serialize_response(response);

    EXPECT_NE(json.find("\"error\""), std::string::npos);
    EXPECT_NE(json.find("\"code\":-32602"), std::string::npos);
    EXPECT_NE(json.find("\"message\""), std::string::npos);
    EXPECT_NE(json.find("\"id\":1"), std::string::npos);
}

// ============================================================================
// 分组 3：请求序列化（客户端侧，3 个，编号 14-16）
// ============================================================================

TEST(ProtocolTest, SerializeRequest) {
    boost::json::array params = {10, 20};
    Request request("add", params, 1);

    std::string json = Protocol::serialize_request(request);

    EXPECT_NE(json.find("\"jsonrpc\":\"2.0\""), std::string::npos);
    EXPECT_NE(json.find("\"method\":\"add\""), std::string::npos);
    EXPECT_NE(json.find("\"params\""), std::string::npos);
    EXPECT_NE(json.find("\"id\":1"), std::string::npos);
}

TEST(ProtocolTest, SerializeBatchRequest) {
    std::vector<Request> requests;

    boost::json::array params1 = {1, 2};
    requests.push_back(Request("add", params1, 1));

    boost::json::array params2 = {5, 3};
    requests.push_back(Request("subtract", params2, 2));

    std::string json = Protocol::serialize_batch_request(requests);

    // 应该是数组
    EXPECT_EQ(json[0], '[');
    EXPECT_EQ(json[json.length() - 1], ']');

    // 应该包含两个请求
    EXPECT_NE(json.find("\"method\":\"add\""), std::string::npos);
    EXPECT_NE(json.find("\"method\":\"subtract\""), std::string::npos);
}

TEST(ProtocolTest, SerializeNotification) {
    boost::json::array params = {"hello"};
    Request notification("update", params);  // 无 ID 的通知

    std::string json = Protocol::serialize_request(notification);

    EXPECT_NE(json.find("\"jsonrpc\":\"2.0\""), std::string::npos);
    EXPECT_NE(json.find("\"method\":\"update\""), std::string::npos);
    EXPECT_NE(json.find("\"params\""), std::string::npos);
    // 通知不应该包含 id 字段
    EXPECT_EQ(json.find("\"id\""), std::string::npos);
}

// ============================================================================
// 分组 4：验证和错误处理（4 个，编号 17-20）
// ============================================================================

TEST(ProtocolTest, ParseInvalidJson) {
    std::string invalid_json = R"({"jsonrpc":"2.0","method":"test",)";  // 缺少结束括号

    EXPECT_THROW({
        Protocol::parse_request(invalid_json);
    }, jsonrpc::Error);
}

TEST(ProtocolTest, ParseMissingMethod) {
    std::string payload = R"({"jsonrpc":"2.0","params":[],"id":1})";  // 缺少 method 字段

    EXPECT_THROW({
        Protocol::parse_request(payload);
    }, jsonrpc::Error);
}

TEST(ProtocolTest, ParseInvalidVersion) {
    std::string payload = R"({"jsonrpc":"1.0","method":"test","id":1})";  // 错误的版本

    EXPECT_THROW({
        Protocol::parse_request(payload);
    }, jsonrpc::Error);
}

TEST(ProtocolTest, IsBatchRequest) {
    // 测试批量请求检测
    std::string batch_json = R"([{"jsonrpc":"2.0","method":"test","id":1}])";
    auto batch_value = boost::json::parse(batch_json);
    EXPECT_TRUE(Protocol::is_batch_request(batch_value));

    // 测试单个请求检测
    std::string single_json = R"({"jsonrpc":"2.0","method":"test","id":1})";
    auto single_value = boost::json::parse(single_json);
    EXPECT_FALSE(Protocol::is_batch_request(single_value));
}
