#include <jsonrpc/jsonrpc.hpp>
#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <memory>
#include <thread>
#include <vector>
#include <map>

using namespace jsonrpc;

// ============================================================================
// 集成测试 Fixture
// ============================================================================

class IntegrationTestFixture : public ::testing::Test {
protected:
    void SetUp() override {
        // 创建服务器，使用端口 19091 避免冲突
        server_.reset(new Server(19091, "127.0.0.1"));

        // 注册测试方法
        server_->register_method("add", [](int a, int b) -> int {
            return a + b;
        });

        server_->register_method("subtract", [](int a, int b) -> int {
            return a - b;
        });

        server_->register_method("multiply", [](int a, int b) -> int {
            return a * b;
        });

        server_->register_method("echo", [](const std::string& msg) -> std::string {
            return msg;
        });

        server_->register_method("delay", [](int millis) -> int {
            std::this_thread::sleep_for(std::chrono::milliseconds(millis));
            return millis;
        });

        server_->register_method("sum_vector", [](const std::vector<int>& numbers) -> int {
            int sum = 0;
            for (int n : numbers) {
                sum += n;
            }
            return sum;
        });

        server_->register_method("get_map", []() -> std::map<std::string, int> {
            std::map<std::string, int> result;
            result["a"] = 1;
            result["b"] = 2;
            result["c"] = 3;
            return result;
        });

        server_->register_method("throw_error", []() -> int {
            throw Error(ErrorCode::ServerError, "故意抛出的错误");
        });

        // 有状态的计数器方法
        counter_ = std::make_shared<std::atomic<int>>(0);
        server_->register_method("increment", [counter = counter_]() -> int {
            return ++(*counter);
        });

        server_->register_method("get_count", [counter = counter_]() -> int {
            return counter->load();
        });

        // 通知接收计数器
        notify_count_ = std::make_shared<std::atomic<int>>(0);
        server_->register_method("notify_received", [notify_count = notify_count_]() -> void {
            ++(*notify_count);
        });

        // 异步启动服务器
        server_->start();
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

    void TearDown() override {
        if (server_) {
            server_->stop();
        }
    }

    std::unique_ptr<Server> server_;
    std::shared_ptr<std::atomic<int>> counter_;
    std::shared_ptr<std::atomic<int>> notify_count_;
};

// ============================================================================
// 分组 1：端到端通信测试（3 个）
// ============================================================================

TEST_F(IntegrationTestFixture, BasicEndToEnd) {
    Client client("127.0.0.1", 19091);

    // 调用 add 方法
    int result = client.call<int>("add", 10, 20);
    EXPECT_EQ(result, 30);
}

TEST_F(IntegrationTestFixture, MultipleClientsEndToEnd) {
    // 创建 3 个客户端同时调用
    std::vector<std::thread> threads;
    std::vector<int> results(3, 0);

    for (int i = 0; i < 3; i++) {
        threads.emplace_back([&results, i]() {
            Client client("127.0.0.1", 19091);
            results[i] = client.call<int>("add", i * 10, i * 5);
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(results[0], 0);   // 0 + 0
    EXPECT_EQ(results[1], 15);  // 10 + 5
    EXPECT_EQ(results[2], 30);  // 20 + 10
}

TEST_F(IntegrationTestFixture, ComplexTypesEndToEnd) {
    Client client("127.0.0.1", 19091);

    // 测试 vector 参数
    std::vector<int> numbers = {1, 2, 3, 4, 5};
    int sum = client.call<int>("sum_vector", numbers);
    EXPECT_EQ(sum, 15);

    // 测试 map 返回值
    auto map_result = client.call<std::map<std::string, int>>("get_map");
    EXPECT_EQ(map_result["a"], 1);
    EXPECT_EQ(map_result["b"], 2);
    EXPECT_EQ(map_result["c"], 3);
}

// ============================================================================
// 分组 2：并发和压力测试（3 个）
// ============================================================================

TEST_F(IntegrationTestFixture, ConcurrentClients) {
    // 10 个客户端并发调用
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};

    for (int i = 0; i < 10; i++) {
        threads.emplace_back([&success_count, i]() {
            Client client("127.0.0.1", 19091);
            int result = client.call<int>("multiply", i, 2);
            if (result == i * 2) {
                ++success_count;
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(success_count.load(), 10);
}

TEST_F(IntegrationTestFixture, HighConcurrency) {
    // 10 个客户端，每个发送 10 个请求，共 100 个并发请求
    std::vector<std::thread> threads;
    std::atomic<int> success_count{0};

    for (int client_id = 0; client_id < 10; client_id++) {
        threads.emplace_back([&success_count]() {
            Client client("127.0.0.1", 19091);
            for (int i = 0; i < 10; i++) {
                int result = client.call<int>("add", i, i);
                if (result == i * 2) {
                    ++success_count;
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(success_count.load(), 100);
}

TEST_F(IntegrationTestFixture, BatchRequestParallel) {
    Client client("127.0.0.1", 19091);

    // 构建包含 10 个延迟请求的批量调用
    std::vector<Request> requests;
    for (int i = 0; i < 10; i++) {
        boost::json::array params = {50};  // 每个请求延迟 50ms
        requests.push_back(Request("delay", params, i));
    }

    // 测量批量请求时间
    auto start = std::chrono::steady_clock::now();

    // 发送批量请求（应该并行处理）
    std::vector<Response> responses = client.call_batch(requests);

    auto end = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    // 验证响应数量
    EXPECT_EQ(responses.size(), 10u);

    // 验证响应顺序（应该保持请求顺序）
    for (size_t i = 0; i < responses.size(); i++) {
        ASSERT_FALSE(responses[i].is_error());
        EXPECT_EQ(responses[i].id().as_int64(), static_cast<int64_t>(i));
        EXPECT_EQ(responses[i].result().as_int64(), 50);
    }

    // 验证并行执行（如果顺序执行需要 500ms，并行执行应该接近 50ms）
    // 允许一些开销，所以检查是否小于 300ms
    EXPECT_LT(duration, 300);
}

// ============================================================================
// 分组 3：长连接和稳定性测试（3 个）
// ============================================================================

TEST_F(IntegrationTestFixture, KeepAliveMultipleRequests) {
    Client client("127.0.0.1", 19091);

    // 连续发送 50 个请求（应该复用同一连接）
    for (int i = 0; i < 50; i++) {
        int result = client.call<int>("add", i, 1);
        EXPECT_EQ(result, i + 1);
    }
}

TEST_F(IntegrationTestFixture, LongRunningConnection) {
    Client client("127.0.0.1", 19091);

    // 保持连接 5 秒，每秒发送一个请求
    for (int i = 0; i < 5; i++) {
        std::string msg = client.call<std::string>("echo", std::string("message_") + std::to_string(i));
        EXPECT_EQ(msg, "message_" + std::to_string(i));

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

TEST_F(IntegrationTestFixture, ReconnectionAfterIdle) {
    Client client("127.0.0.1", 19091);

    // 发送第一个请求
    int first = client.call<int>("add", 5, 5);
    EXPECT_EQ(first, 10);

    // 空闲 3 秒
    std::this_thread::sleep_for(std::chrono::seconds(3));

    // 发送第二个请求（可能需要重连）
    int second = client.call<int>("subtract", 20, 8);
    EXPECT_EQ(second, 12);
}

// ============================================================================
// 分组 4：错误和异常测试（3 个）
// ============================================================================

TEST_F(IntegrationTestFixture, ServerMethodError) {
    Client client("127.0.0.1", 19091);

    // 调用会抛出异常的方法
    EXPECT_THROW(client.call<int>("throw_error"), Error);
}

TEST_F(IntegrationTestFixture, MethodNotFoundError) {
    Client client("127.0.0.1", 19091);

    // 调用未注册的方法
    EXPECT_THROW(client.call<int>("non_existent_method", 1, 2), Error);
}

TEST_F(IntegrationTestFixture, InvalidParamsError) {
    Client client("127.0.0.1", 19091);

    // 调用方法但参数类型错误（add 需要 int，传递 string）
    EXPECT_THROW(client.call<int>("add", std::string("invalid"), 2), Error);
}

// ============================================================================
// 分组 5：功能集成测试（3 个）
// ============================================================================

TEST_F(IntegrationTestFixture, NotificationIntegration) {
    Client client("127.0.0.1", 19091);

    // 记录初始计数
    int initial_count = notify_count_->load();

    // 发送 5 个通知
    for (int i = 0; i < 5; i++) {
        client.notify("notify_received");
    }

    // 等待通知处理完成
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // 验证通知被执行（但客户端不等待响应）
    int final_count = notify_count_->load();
    EXPECT_EQ(final_count - initial_count, 5);
}

TEST_F(IntegrationTestFixture, MixedBatchRequest) {
    Client client("127.0.0.1", 19091);

    // 构建混合批量请求（包含请求和通知）
    std::vector<Request> requests;

    // 添加 3 个普通请求（有 ID）
    requests.push_back(Request("add", boost::json::array{10, 20}, 1));
    requests.push_back(Request("subtract", boost::json::array{50, 30}, 2));
    requests.push_back(Request("multiply", boost::json::array{5, 6}, 3));

    // 添加 2 个通知（无 ID）
    requests.push_back(Request("notify_received", boost::json::array{}));
    requests.push_back(Request("notify_received", boost::json::array{}));

    int initial_notify_count = notify_count_->load();

    // 发送混合批量请求
    std::vector<Response> responses = client.call_batch(requests);

    // 只应该返回 3 个响应（通知不返回响应）
    EXPECT_EQ(responses.size(), 3u);

    // 验证响应内容
    ASSERT_FALSE(responses[0].is_error());
    EXPECT_EQ(responses[0].result().as_int64(), 30);

    ASSERT_FALSE(responses[1].is_error());
    EXPECT_EQ(responses[1].result().as_int64(), 20);

    ASSERT_FALSE(responses[2].is_error());
    EXPECT_EQ(responses[2].result().as_int64(), 30);

    // 等待通知处理
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // 验证通知被执行
    int final_notify_count = notify_count_->load();
    EXPECT_EQ(final_notify_count - initial_notify_count, 2);
}

TEST_F(IntegrationTestFixture, StateManagement) {
    Client client("127.0.0.1", 19091);

    // 调用 increment 5 次
    for (int i = 0; i < 5; i++) {
        int value = client.call<int>("increment");
        EXPECT_EQ(value, i + 1);
    }

    // 验证计数器状态
    int count = client.call<int>("get_count");
    EXPECT_EQ(count, 5);

    // 再次 increment
    int next = client.call<int>("increment");
    EXPECT_EQ(next, 6);
}
