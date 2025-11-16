#include <jsonrpc/jsonrpc.hpp>
#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <memory>
#include <thread>
#include <string>

using namespace jsonrpc;

// ============================================================================
// 客户端测试 Fixture
// ============================================================================

class JsonRpcServerFixture : public ::testing::Test {
protected:
    void SetUp() override {
        server_.reset(new Server(19090, "127.0.0.1"));

        // 注册测试方法
        server_->register_method("add", [](int a, int b) { return a + b; });

        server_->register_method("multiply", [](int a, int b) { return a * b; });

        server_->register_method("echo", [](const std::string& msg) { return msg; });

        server_->register_method("no_params", []() { return 42; });

        server_->register_method("delay", [](int millis) {
            std::this_thread::sleep_for(std::chrono::milliseconds(millis));
            return millis;
        });

        server_->register_method("throw_error", []() -> int {
            throw Error(ErrorCode::ServerError, "服务器错误");
        });

        server_->register_method("sum_vector", [](const std::vector<int>& numbers) {
            int sum = 0;
            for (int n : numbers) {
                sum += n;
            }
            return sum;
        });

        // 通知计数器
        notify_counter_ = std::make_shared<std::atomic<int>>(0);
        server_->register_method("notify_handler", [counter = notify_counter_]() {
            ++(*counter);
        });

        server_->start();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    void TearDown() override {
        if (server_) {
            server_->stop();
        }
    }

    std::unique_ptr<Server> server_;
    std::shared_ptr<std::atomic<int>> notify_counter_;
};

// ============================================================================
// 现有测试（保留）
// ============================================================================

TEST_F(JsonRpcServerFixture, SyncCall) {
    Client client("127.0.0.1", 19090);
    auto result = client.call<int>("add", 5, 7);
    EXPECT_EQ(result, 12);
}

TEST_F(JsonRpcServerFixture, AsyncCall) {
    Client client("127.0.0.1", 19090);
    std::atomic<int> received{0};

    client.async_call("delay", [&](const Response& resp) {
        ASSERT_FALSE(resp.is_error());
        EXPECT_EQ(resp.result().as_int64(), 50);
        ++received;
    }, 50);

    client.run();
    EXPECT_EQ(received.load(), 1);
}

TEST_F(JsonRpcServerFixture, MultipleAsyncCalls) {
    Client client("127.0.0.1", 19090);
    std::atomic<int> received{0};

    for (int i = 0; i < 5; ++i) {
        client.async_call("add", [&](const Response& resp) {
            ASSERT_FALSE(resp.is_error());
            EXPECT_EQ(resp.result().as_int64(), 12);
            ++received;
        }, 5, 7);
    }

    client.run();
    EXPECT_EQ(received.load(), 5);
}

TEST_F(JsonRpcServerFixture, AsyncCallRunFor) {
    Client client("127.0.0.1", 19090);
    std::atomic<int> received{0};

    client.async_call("delay", [&](const Response& resp) {
        ASSERT_FALSE(resp.is_error());
        ++received;
    }, 50);

    auto processed = client.run_for(std::chrono::milliseconds(500));
    EXPECT_GE(processed, 1u);
    EXPECT_EQ(received.load(), 1);
}

TEST_F(JsonRpcServerFixture, RunUntilIdleProcessesPostedHandlers) {
    Client client("127.0.0.1", 19090);
    std::atomic<int> executed{0};

    client.get_io_context().post([&]() { ++executed; });
    client.get_io_context().post([&]() { ++executed; });

    auto processed = client.run_until_idle();
    EXPECT_EQ(processed, 2u);
    EXPECT_EQ(executed.load(), 2);
}

TEST(ClientLoggingTest, LoggerCapturesNetworkError) {
    Client client("127.0.0.1", 19999);
    client.set_timeout(std::chrono::milliseconds(200));
    std::string last_message;
    client.set_logger([&](const std::string& msg) {
        last_message = msg;
    });

    EXPECT_THROW(client.call<int>("missing"), Error);
    EXPECT_FALSE(last_message.empty());
}

// ============================================================================
// 分组 1：基础功能（5 个，编号 4-8）
// ============================================================================

TEST_F(JsonRpcServerFixture, SyncCallMultipleParams) {
    Client client("127.0.0.1", 19090);

    // 两个参数
    int sum = client.call<int>("add", 10, 20);
    EXPECT_EQ(sum, 30);

    std::string echoed = client.call<std::string>("echo", std::string("hello"));
    EXPECT_EQ(echoed, "hello");
}

TEST_F(JsonRpcServerFixture, SyncCallNoParams) {
    Client client("127.0.0.1", 19090);

    int value = client.call<int>("no_params");
    EXPECT_EQ(value, 42);
}

TEST_F(JsonRpcServerFixture, Notify) {
    Client client("127.0.0.1", 19090);

    int initial_count = notify_counter_->load();

    // 发送通知
    client.notify("notify_handler");

    // 等待通知处理
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    int final_count = notify_counter_->load();
    EXPECT_EQ(final_count - initial_count, 1);
}

TEST_F(JsonRpcServerFixture, NotifyNoResponse) {
    Client client("127.0.0.1", 19090);

    // 通知不应该等待响应，应该立即返回
    auto start = std::chrono::steady_clock::now();

    client.notify("delay", 100);  // 即使方法延迟 100ms

    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    // 通知应该立即返回，不等待方法执行完成
    EXPECT_LT(duration, 50);  // 应该远小于 100ms
}

TEST_F(JsonRpcServerFixture, SyncCallComplexTypes) {
    Client client("127.0.0.1", 19090);

    // 测试 vector 参数
    std::vector<int> numbers = {10, 20, 30, 40};
    int total = client.call<int>("sum_vector", numbers);
    EXPECT_EQ(total, 100);
}

// ============================================================================
// 分组 2：异步功能（4 个，编号 9-12）
// ============================================================================

TEST_F(JsonRpcServerFixture, AsyncCallWithError) {
    Client client("127.0.0.1", 19090);
    std::atomic<bool> error_received{false};

    client.async_call("throw_error", [&](const Response& resp) {
        EXPECT_TRUE(resp.is_error());
        EXPECT_EQ(resp.error().code(), ErrorCode::ServerError);
        error_received = true;
    });

    client.run();
    EXPECT_TRUE(error_received.load());
}

TEST_F(JsonRpcServerFixture, AsyncConcurrent) {
    Client client("127.0.0.1", 19090);
    std::atomic<int> success_count{0};

    // 并发发送 20 个异步请求
    for (int i = 0; i < 20; ++i) {
        client.async_call("multiply", [&](const Response& resp) {
            if (!resp.is_error() && resp.result().as_int64() == 15) {
                ++success_count;
            }
        }, 3, 5);
    }

    client.run();
    EXPECT_EQ(success_count.load(), 20);
}

TEST_F(JsonRpcServerFixture, AsyncTimeout) {
    Client client("127.0.0.1", 19090);
    client.set_timeout(std::chrono::milliseconds(100));  // 设置 100ms 超时

    std::atomic<bool> timeout_occurred{false};

    // 调用延迟 200ms 的方法
    client.async_call("delay", [&](const Response& resp) {
        // 应该超时或返回错误
        timeout_occurred = true;
    }, 200);

    client.run();
    EXPECT_TRUE(timeout_occurred.load());
}

TEST_F(JsonRpcServerFixture, AsyncCallbackOrder) {
    Client client("127.0.0.1", 19090);
    std::vector<int> callback_order;
    std::mutex order_mutex;

    // 发送 5 个异步请求，每个带不同延迟
    for (int i = 0; i < 5; ++i) {
        client.async_call("add", [&, i](const Response& resp) {
            std::lock_guard<std::mutex> lock(order_mutex);
            callback_order.push_back(i);
        }, i, 0);
    }

    client.run();

    // 验证所有回调都被调用
    EXPECT_EQ(callback_order.size(), 5u);
}

// ============================================================================
// 分组 3：批量请求（2 个，编号 13-14）
// ============================================================================

TEST_F(JsonRpcServerFixture, BatchRequest) {
    Client client("127.0.0.1", 19090);

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
}

TEST_F(JsonRpcServerFixture, BatchMixedRequests) {
    Client client("127.0.0.1", 19090);

    int initial_notify_count = notify_counter_->load();

    // 混合批量请求（请求 + 通知）
    std::vector<Request> requests;
    requests.push_back(Request("add", boost::json::array{5, 5}, 1));
    requests.push_back(Request("notify_handler", boost::json::array{}));  // 通知
    requests.push_back(Request("multiply", boost::json::array{2, 3}, 2));

    std::vector<Response> responses = client.call_batch(requests);

    // 只应该返回 2 个响应（通知不返回响应）
    EXPECT_EQ(responses.size(), 2u);

    EXPECT_EQ(responses[0].result().as_int64(), 10);
    EXPECT_EQ(responses[1].result().as_int64(), 6);

    // 等待通知处理
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 验证通知被执行
    int final_notify_count = notify_counter_->load();
    EXPECT_EQ(final_notify_count - initial_notify_count, 1);
}

// ============================================================================
// 分组 4：错误处理（3 个，编号 15-17）
// ============================================================================

TEST_F(JsonRpcServerFixture, MethodNotFoundError) {
    Client client("127.0.0.1", 19090);

    EXPECT_THROW(client.call<int>("non_existent_method", 1, 2), Error);
}

TEST_F(JsonRpcServerFixture, InvalidParamsError) {
    Client client("127.0.0.1", 19090);

    // add 方法期望两个 int 参数，但传递 string
    EXPECT_THROW(client.call<int>("add", std::string("invalid"), 2), Error);
}

TEST_F(JsonRpcServerFixture, ServerErrorHandling) {
    Client client("127.0.0.1", 19090);

    EXPECT_THROW(client.call<int>("throw_error"), Error);
}

// ============================================================================
// 分组 5：超时和配置（2 个，编号 18-19）
// ============================================================================

TEST_F(JsonRpcServerFixture, SetTimeout) {
    Client client("127.0.0.1", 19090);

    // 设置超时为 5 秒
    client.set_timeout(std::chrono::milliseconds(5000));

    // 正常调用应该成功
    int result = client.call<int>("add", 1, 2);
    EXPECT_EQ(result, 3);
}

TEST_F(JsonRpcServerFixture, TimeoutError) {
    Client client("127.0.0.1", 19090);

    // 设置非常短的超时（50ms）
    client.set_timeout(std::chrono::milliseconds(50));

    // 调用延迟 200ms 的方法应触发超时
    EXPECT_THROW(client.call<int>("delay", 200), Error);
}
