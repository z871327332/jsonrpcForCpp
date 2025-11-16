#include <jsonrpc/detail/method_registry.hpp>
#include <jsonrpc/server.hpp>
#include <jsonrpc/client.hpp>
#include <jsonrpc/types.hpp>
#include <gtest/gtest.h>
#include <boost/asio.hpp>
#include <chrono>
#include <thread>
#include <atomic>
#include <vector>

using namespace jsonrpc;
using namespace jsonrpc::detail;

// ============================================================================
// 现有测试（保留）
// ============================================================================

TEST(ServerTest, MethodRegistryInvoke) {
    MethodRegistry registry;
    registry.register_method("add", [](int a, int b) { return a + b; });

    Request request("add", boost::json::array{1, 2}, boost::json::value(1));
    auto response = registry.invoke(request);
    ASSERT_FALSE(response.is_error());
    EXPECT_EQ(response.result().as_int64(), 3);
}

TEST(ServerTest, BatchInvokeHandlesNotifications) {
    MethodRegistry registry;
    registry.register_method("echo", [](int value) { return value; });

    std::vector<Request> requests;
    requests.emplace_back("echo", boost::json::array{5}, boost::json::value(10));
    requests.emplace_back("echo", boost::json::array{7});  // notification
    requests.emplace_back("echo", boost::json::array{9}, boost::json::value(11));

    auto responses = registry.invoke_batch(requests);
    ASSERT_EQ(responses.size(), 2u);
    EXPECT_EQ(responses[0].result().as_int64(), 5);
    EXPECT_EQ(responses[1].result().as_int64(), 9);
}

TEST(ServerTest, ConfigurableBatchConcurrency) {
    MethodRegistry registry;
    registry.set_batch_concurrency(1);
    registry.register_method("square", [](int value) { return value * value; });

    std::vector<Request> requests;
    for (int i = 0; i < 4; ++i) {
        requests.emplace_back("square", boost::json::array{i}, boost::json::value(i));
    }

    auto responses = registry.invoke_batch(requests);
    ASSERT_EQ(responses.size(), 4u);
    EXPECT_EQ(responses[3].result().as_int64(), 9);
}

TEST(ServerApiTest, SetBatchConcurrencyRequiresStoppedServer) {
    Server server(19191);
    server.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_THROW(server.set_batch_concurrency(2), std::logic_error);
    server.stop();
    EXPECT_NO_THROW(server.set_batch_concurrency(2));
}

TEST(ServerApiTest, ServerCanRestartAfterStop) {
    Server server(19192);
    server.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    server.stop();
    EXPECT_NO_THROW(server.start());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    server.stop();
}

TEST(ServerApiTest, LoggerCapturesInvalidRequest) {
    Server server(19193);
    std::vector<std::string> logs;
    server.set_logger([&](const std::string& msg) {
        logs.push_back(msg);
    });
    server.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    boost::asio::io_context io;
    boost::asio::ip::tcp::socket socket(io);
    socket.connect({boost::asio::ip::make_address("127.0.0.1"), 19193});
    std::string request = "GET / HTTP/1.1\r\nHost: 127.0.0.1\r\n\r\n";
    boost::asio::write(socket, boost::asio::buffer(request));

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    server.stop();

    EXPECT_FALSE(logs.empty());
}

TEST(ServerTest, NotificationErrorsDoNotProduceResponses) {
    MethodRegistry registry;
    registry.register_method("boom", []() -> int {
        throw Error(ErrorCode::InternalError, "boom");
    });

    std::vector<Request> requests;
    requests.emplace_back("boom", boost::json::array{}, boost::json::value(1));
    requests.emplace_back("boom", boost::json::array{});

    auto responses = registry.invoke_batch(requests);
    ASSERT_EQ(responses.size(), 1u);
    EXPECT_TRUE(responses[0].is_error());
    EXPECT_EQ(responses[0].id().as_int64(), 1);
}

// ============================================================================
// 分组 1：启动/停止和注册（4 个，编号 5-8）
// ============================================================================

TEST(ServerApiTest, StartStop) {
    // 测试服务器启动和停止
    Server server(19192, "127.0.0.1");

    server.register_method("test", []() { return 42; });

    // 启动服务器
    EXPECT_NO_THROW(server.start());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 验证服务器正在运行（通过客户端连接）
    Client client("127.0.0.1", 19192);
    int value = client.call<int>("test");
    EXPECT_EQ(value, 42);

    // 停止服务器
    EXPECT_NO_THROW(server.stop());
}

TEST(ServerApiTest, MultipleStarts) {
    // 测试多次启动服务器（第二次应抛出异常）
    Server server(19193, "127.0.0.1");

    server.register_method("test", []() { return 1; });

    // 第一次启动
    EXPECT_NO_THROW(server.start());
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 第二次启动应该抛出异常
    EXPECT_THROW(server.start(), std::logic_error);

    server.stop();
}

TEST(ServerApiTest, RegisterMultipleMethods) {
    // 测试注册多个方法
    Server server(19194, "127.0.0.1");

    // 注册多个不同签名的方法
    server.register_method("add", [](int a, int b) { return a + b; });
    server.register_method("multiply", [](int a, int b) { return a * b; });
    server.register_method("echo", [](const std::string& msg) { return msg; });
    server.register_method("get_constant", []() { return 100; });

    server.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 验证所有方法都可用
    Client client("127.0.0.1", 19194);

    auto resp1 = client.call<int>("add", 10, 20);
    EXPECT_EQ(resp1, 30);

    auto resp2 = client.call<int>("multiply", 5, 6);
    EXPECT_EQ(resp2, 30);

    auto resp3 = client.call<std::string>("echo", std::string("hello"));
    EXPECT_EQ(resp3, "hello");

    auto resp4 = client.call<int>("get_constant");
    EXPECT_EQ(resp4, 100);

    server.stop();
}

TEST(ServerApiTest, RegisterDuplicateMethod) {
    // 测试注册重复方法名（应该覆盖）
    Server server(19195, "127.0.0.1");

    // 注册第一个实现
    server.register_method("test", []() { return 1; });

    // 注册同名方法（覆盖）
    server.register_method("test", []() { return 2; });

    server.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 验证使用的是第二个实现
    Client client("127.0.0.1", 19195);
    int result = client.call<int>("test");
    EXPECT_EQ(result, 2);

    server.stop();
}

// ============================================================================
// 分组 2：请求处理（3 个，编号 9-11）
// ============================================================================

TEST(ServerApiTest, UnregisteredMethodCall) {
    // 测试调用未注册的方法
    Server server(19196, "127.0.0.1");

    server.register_method("registered", []() { return 1; });

    server.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    Client client("127.0.0.1", 19196);

    // 调用未注册的方法
    EXPECT_THROW(client.call<int>("unregistered", 1, 2), Error);

    server.stop();
}

TEST(ServerApiTest, BatchRequest) {
    // 测试批量请求处理
    Server server(19197, "127.0.0.1");

    server.register_method("add", [](int a, int b) { return a + b; });
    server.register_method("multiply", [](int a, int b) { return a * b; });

    server.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    Client client("127.0.0.1", 19197);

    // 构建批量请求
    std::vector<Request> requests;
    requests.push_back(Request("add", boost::json::array{10, 20}, 1));
    requests.push_back(Request("multiply", boost::json::array{5, 6}, 2));
    requests.push_back(Request("add", boost::json::array{1, 1}, 3));

    std::vector<Response> responses = client.call_batch(requests);

    ASSERT_EQ(responses.size(), 3u);

    EXPECT_FALSE(responses[0].is_error());
    EXPECT_EQ(responses[0].result().as_int64(), 30);

    EXPECT_FALSE(responses[1].is_error());
    EXPECT_EQ(responses[1].result().as_int64(), 30);

    EXPECT_FALSE(responses[2].is_error());
    EXPECT_EQ(responses[2].result().as_int64(), 2);

    server.stop();
}

TEST(ServerApiTest, NotificationHandling) {
    // 测试通知处理
    Server server(19198, "127.0.0.1");

    std::shared_ptr<std::atomic<int>> counter = std::make_shared<std::atomic<int>>(0);

    server.register_method("increment", [counter]() {
        ++(*counter);
    });

    server.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    Client client("127.0.0.1", 19198);

    int initial_count = counter->load();

    // 发送 5 个通知
    for (int i = 0; i < 5; i++) {
        client.notify("increment");
    }

    // 等待通知处理完成
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    int final_count = counter->load();
    EXPECT_EQ(final_count - initial_count, 5);

    server.stop();
}

// ============================================================================
// 分组 3：高级功能（3 个，编号 12-14）
// ============================================================================

TEST(ServerApiTest, ConcurrentRequests) {
    // 测试并发请求处理
    Server server(19199, "127.0.0.1");

    server.register_method("add", [](int a, int b) { return a + b; });

    server.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 启动 5 个客户端并发调用
    std::vector<std::thread> threads;
    std::vector<int> results(5, 0);

    for (int i = 0; i < 5; i++) {
        threads.emplace_back([&results, i]() {
            Client client("127.0.0.1", 19199);
            int value = client.call<int>("add", i * 10, i * 5);
            results[i] = value;
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // 验证所有请求都正确处理
    EXPECT_EQ(results[0], 0);   // 0 + 0
    EXPECT_EQ(results[1], 15);  // 10 + 5
    EXPECT_EQ(results[2], 30);  // 20 + 10
    EXPECT_EQ(results[3], 45);  // 30 + 15
    EXPECT_EQ(results[4], 60);  // 40 + 20

    server.stop();
}

TEST(ServerApiTest, MethodException) {
    // 测试方法抛出异常的处理
    Server server(19200, "127.0.0.1");

    server.register_method("throw_error", []() -> int {
        throw Error(ErrorCode::ServerError, "方法执行错误");
    });

    server.register_method("normal", []() { return 42; });

    server.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    Client client("127.0.0.1", 19200);

    // 调用会抛出异常的方法
    try {
        (void)client.call<int>("throw_error");
        FAIL() << "call 应该抛出 Error";
    } catch (const Error& e) {
        EXPECT_EQ(e.code(), ErrorCode::ServerError);
        EXPECT_EQ(e.message(), "方法执行错误");
    }

    // 验证服务器仍然正常工作
    int normal = client.call<int>("normal");
    EXPECT_EQ(normal, 42);

    server.stop();
}

TEST(ServerApiTest, KeepAlive) {
    // 测试长连接保持
    Server server(19201, "127.0.0.1");

    server.register_method("echo", [](int value) { return value; });

    server.start();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    Client client("127.0.0.1", 19201);

    // 在同一连接上发送多个请求（间隔 500ms）
    for (int i = 0; i < 5; i++) {
        int value = client.call<int>("echo", i);
        EXPECT_EQ(value, i);

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    server.stop();
}
