# JsonRPC 使用教程

本教程将详细介绍 JsonRPC 库的各项功能和使用方法，从基础的服务器和客户端开发，到高级的类型系统和并发处理。

## 第一部分：服务器开发指南

### 1.1 创建基础服务器

创建一个 JSON-RPC 服务器非常简单，只需三步：

```cpp
#include <jsonrpc/jsonrpc.hpp>

using namespace jsonrpc;

int main() {
    // 1. 创建服务器实例
    Server server("127.0.0.1", 8080);

    // 2. 注册方法
    server.register_method("hello", []() -> std::string {
        return "Hello, World!";
    });

    // 3. 启动服务器
    server.run();  // 阻塞运行

    return 0;
}
```

服务器将监听 `127.0.0.1:8080`，并处理 JSON-RPC 请求。

### 1.2 注册不同类型的方法

#### 无参数方法

```cpp
server.register_method("ping", []() -> std::string {
    return "pong";
});
```

#### 带参数方法

JsonRPC 支持任意数量和类型的参数，参数类型会自动从 JSON 转换：

```cpp
server.register_method("add", [](int a, int b) -> int {
    return a + b;
});

server.register_method("greet", [](const std::string& name) -> std::string {
    return "Hello, " + name + "!";
});

server.register_method("sum_vector", [](const std::vector<int>& numbers) -> int {
    int sum = 0;
    for (int n : numbers) {
        sum += n;
    }
    return sum;
});
```

#### 无返回值方法

对于不需要返回值的方法，使用 `void` 返回类型：

```cpp
server.register_method("log", [](const std::string& message) -> void {
    std::cout << "[LOG] " << message << std::endl;
});
```

#### 复杂参数类型

支持嵌套容器和多种类型混合：

```cpp
server.register_method("process_data", [](
    const std::string& name,
    const std::vector<int>& values,
    const std::map<std::string, std::string>& options
) -> std::map<std::string, int> {
    std::map<std::string, int> result;
    result["count"] = values.size();
    result["sum"] = std::accumulate(values.begin(), values.end(), 0);
    return result;
});
```

### 1.3 错误处理

在方法实现中，可以通过抛出 `Error` 对象来返回错误：

```cpp
server.register_method("divide", [](int a, int b) -> double {
    if (b == 0) {
        throw Error(ErrorCode::InvalidParams, "除数不能为零");
    }
    return static_cast<double>(a) / b;
});

server.register_method("get_user", [](int user_id) -> std::map<std::string, std::string> {
    if (user_id < 0) {
        throw Error(ErrorCode::InvalidParams, "用户 ID 必须为非负数");
    }

    // 假设用户不存在
    if (user_id == 999) {
        throw Error(ErrorCode::ServerError, "用户不存在");
    }

    std::map<std::string, std::string> user;
    user["id"] = std::to_string(user_id);
    user["name"] = "用户" + std::to_string(user_id);
    return user;
});
```

标准错误代码包括：
- `ParseError` (-32700)：JSON 解析错误
- `InvalidRequest` (-32600)：无效的请求对象
- `MethodNotFound` (-32601)：方法不存在
- `InvalidParams` (-32602)：参数无效
- `InternalError` (-32603)：内部错误
- `ServerError` (-32000 到 -32099)：服务器自定义错误

### 1.4 启动和停止服务器

#### 阻塞运行

```cpp
server.run();  // 阻塞，直到服务器停止
```

#### 异步启动

```cpp
server.start();  // 异步启动，立即返回

// 执行其他操作...

// 保持主线程运行
while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
}
```

#### 停止服务器

```cpp
server.stop();  // 停止服务器
```

#### 信号处理

```cpp
#include <csignal>

Server* g_server = nullptr;

void signal_handler(int signal) {
    if (g_server) {
        g_server->stop();
    }
}

int main() {
    Server server("127.0.0.1", 8080);
    g_server = &server;

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    // 注册方法...

    server.run();
    return 0;
}
```

## 第二部分：客户端开发指南

### 2.1 创建客户端

```cpp
#include <jsonrpc/jsonrpc.hpp>

using namespace jsonrpc;

int main() {
    // 创建客户端，连接到服务器
    Client client("127.0.0.1", 8080);

    // 调用远程方法...

    return 0;
}
```

### 2.2 同步调用

同步调用会阻塞直到收到响应或超时：

```cpp
// 简单调用
Response resp = client.call("ping");
if (!resp.is_error()) {
    std::cout << resp.result().as_string() << std::endl;  // "pong"
}

// 带参数调用
Response resp2 = client.call("add", 10, 20);
if (!resp2.is_error()) {
    std::cout << "结果: " << resp2.result().as_int64() << std::endl;  // 30
}

// 多参数调用
Response resp3 = client.call("greet", std::string("张三"));
if (!resp3.is_error()) {
    std::cout << resp3.result().as_string() << std::endl;  // "Hello, 张三!"
}
```

### 2.3 异步调用

异步调用立即返回，通过回调函数处理响应：

```cpp
// 单个异步调用
client.async_call("add", [](const Response& resp) {
    if (!resp.is_error()) {
        std::cout << "结果: " << resp.result().as_int64() << std::endl;
    } else {
        std::cerr << "错误: " << resp.error().message() << std::endl;
    }
}, 10, 20);

// 多个并发异步调用
for (int i = 0; i < 10; i++) {
    client.async_call("process", [i](const Response& resp) {
        std::cout << "请求 " << i << " 完成" << std::endl;
    }, i);
}

// 等待所有异步调用完成
client.run();
```

### 2.4 发送通知

通知是不需要响应的单向消息：

```cpp
// 发送通知，不等待响应
client.notify("log", std::string("这是一条日志消息"));

// 通知适合用于不关心结果的操作
client.notify("update_status", std::string("online"));
```

### 2.5 超时设置

```cpp
// 设置超时为 5 秒
client.set_timeout(5000);

// 超时后会收到错误响应
Response resp = client.call("slow_method");
if (resp.is_error()) {
    std::cerr << "调用失败: " << resp.error().message() << std::endl;
}
```

### 2.6 批量请求

批量请求可以在一次 HTTP 请求中发送多个 RPC 调用：

```cpp
// 构建批量请求
std::vector<Request> requests;
requests.push_back(Request("add", boost::json::array{10, 20}, 1));
requests.push_back(Request("multiply", boost::json::array{5, 6}, 2));
requests.push_back(Request("subtract", boost::json::array{100, 30}, 3));

// 发送批量请求
std::vector<Response> responses = client.call_batch(requests);

// 处理响应
for (const auto& resp : responses) {
    if (!resp.is_error()) {
        std::cout << "结果: " << resp.result().as_int64() << std::endl;
    }
}
```

## 第三部分：类型系统

### 3.1 支持的基本类型

JsonRPC 自动转换以下 C++ 基本类型：

| C++ 类型 | JSON 类型 | 示例 |
|---------|----------|------|
| `int`, `long`, `int64_t` | Number | 42 |
| `double`, `float` | Number | 3.14 |
| `bool` | Boolean | true/false |
| `std::string` | String | "hello" |

### 3.2 容器类型

#### std::vector

```cpp
server.register_method("reverse", [](const std::vector<int>& vec) -> std::vector<int> {
    return std::vector<int>(vec.rbegin(), vec.rend());
});

// 客户端调用
std::vector<int> input = {1, 2, 3, 4, 5};
Response resp = client.call("reverse", input);
```

#### std::map

```cpp
server.register_method("get_config", []() -> std::map<std::string, std::string> {
    std::map<std::string, std::string> config;
    config["host"] = "localhost";
    config["port"] = "8080";
    return config;
});
```

### 3.3 嵌套类型

支持任意深度的嵌套：

```cpp
// 二维数组
server.register_method("matrix_sum", [](
    const std::vector<std::vector<int>>& matrix
) -> int {
    int sum = 0;
    for (const auto& row : matrix) {
        for (int val : row) {
            sum += val;
        }
    }
    return sum;
});

// Map 嵌套 Vector
server.register_method("group_stats", [](
    const std::map<std::string, std::vector<int>>& groups
) -> std::map<std::string, int> {
    std::map<std::string, int> result;
    for (const auto& pair : groups) {
        int sum = std::accumulate(pair.second.begin(), pair.second.end(), 0);
        result[pair.first] = sum;
    }
    return result;
});
```

### 3.4 自定义类型转换

通过特化 `json_converter` 模板，可以支持自定义类型：

```cpp
namespace jsonrpc {
namespace detail {

// 自定义类型
struct Point {
    int x;
    int y;
};

// 特化 json_converter
template<>
struct json_converter<Point> {
    static boost::json::value to_json(const Point& p) {
        boost::json::object obj;
        obj["x"] = p.x;
        obj["y"] = p.y;
        return obj;
    }

    static Point from_json(const boost::json::value& v) {
        Point p;
        p.x = v.at("x").as_int64();
        p.y = v.at("y").as_int64();
        return p;
    }
};

} // namespace detail
} // namespace jsonrpc

// 使用自定义类型
server.register_method("distance", [](const Point& p1, const Point& p2) -> double {
    int dx = p2.x - p1.x;
    int dy = p2.y - p1.y;
    return std::sqrt(dx*dx + dy*dy);
});
```

## 第四部分：错误处理

### 4.1 服务器端错误处理

```cpp
server.register_method("safe_divide", [](double a, double b) -> double {
    try {
        if (b == 0.0) {
            throw Error(ErrorCode::InvalidParams, "除数不能为零");
        }
        return a / b;
    } catch (const Error&) {
        throw;  // 重新抛出 JSON-RPC 错误
    } catch (const std::exception& e) {
        // 其他异常会被自动转换为 InternalError
        throw Error(ErrorCode::InternalError, std::string("计算失败: ") + e.what());
    }
});
```

### 4.2 客户端错误处理

```cpp
Response resp = client.call("divide", 10, 0);

if (resp.is_error()) {
    const Error& err = resp.error();

    std::cerr << "错误码: " << static_cast<int>(err.code()) << std::endl;
    std::cerr << "错误消息: " << err.message() << std::endl;

    // 根据错误码处理
    switch (err.code()) {
        case ErrorCode::InvalidParams:
            std::cerr << "参数错误" << std::endl;
            break;
        case ErrorCode::MethodNotFound:
            std::cerr << "方法不存在" << std::endl;
            break;
        default:
            std::cerr << "其他错误" << std::endl;
    }
}
```

### 4.3 自定义错误

```cpp
// 使用 ServerError 范围 (-32000 到 -32099)
const ErrorCode UserNotFound = static_cast<ErrorCode>(-32001);
const ErrorCode PermissionDenied = static_cast<ErrorCode>(-32002);

server.register_method("delete_user", [](int user_id) -> bool {
    if (user_id == 999) {
        throw Error(UserNotFound, "用户不存在");
    }

    if (user_id == 1) {
        throw Error(PermissionDenied, "不能删除管理员账户");
    }

    // 删除用户...
    return true;
});
```

## 第五部分：高级主题

### 5.1 批量请求并行处理

JsonRPC 使用 `std::async` 实现批量请求的真正并行处理：

```cpp
// 服务器端会并行处理批量请求中的每个调用
std::vector<Request> batch;
for (int i = 0; i < 10; i++) {
    batch.push_back(Request("heavy_computation", boost::json::array{i}, i));
}

// 10 个请求会并行执行，显著提高吞吐量
std::vector<Response> responses = client.call_batch(batch);
```

### 5.2 状态管理

由于方法是无状态的 lambda 或函数对象，需要状态时可以使用捕获：

```cpp
// 使用 shared_ptr 共享状态
auto counter = std::make_shared<std::atomic<int>>(0);

server.register_method("increment", [counter]() -> int {
    return ++(*counter);
});

server.register_method("get_count", [counter]() -> int {
    return counter->load();
});
```

### 5.3 Keep-Alive 连接

JsonRPC 自动支持 HTTP Keep-Alive，客户端和服务器会复用 TCP 连接：

```cpp
Client client("127.0.0.1", 8080);

// 多次调用会复用同一个连接
for (int i = 0; i < 100; i++) {
    Response resp = client.call("ping");
}
```

### 5.4 超时和重试

```cpp
Client client("127.0.0.1", 8080);
client.set_timeout(3000);  // 3 秒超时

int max_retries = 3;
int retry_count = 0;

while (retry_count < max_retries) {
    Response resp = client.call("unstable_method");

    if (!resp.is_error()) {
        std::cout << "成功" << std::endl;
        break;
    }

    if (resp.error().code() == ErrorCode::InternalError) {
        retry_count++;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    } else {
        // 非临时错误，不重试
        break;
    }
}
```

## 第六部分：最佳实践

### 6.1 推荐做法 (DO)

✅ **使用有意义的方法名**
```cpp
server.register_method("calculate_user_score", ...);  // 好
server.register_method("calc", ...);                   // 不好
```

✅ **验证参数**
```cpp
server.register_method("set_age", [](int age) -> bool {
    if (age < 0 || age > 150) {
        throw Error(ErrorCode::InvalidParams, "年龄必须在 0-150 之间");
    }
    return true;
});
```

✅ **使用类型安全的参数**
```cpp
server.register_method("add", [](int a, int b) -> int { ... });  // 好
// 避免使用 boost::json::value 作为参数，失去类型安全
```

✅ **处理异常**
```cpp
server.register_method("safe_op", []() -> int {
    try {
        // 可能抛出异常的操作
    } catch (const std::exception& e) {
        throw Error(ErrorCode::InternalError, e.what());
    }
});
```

✅ **设置合理的超时**
```cpp
client.set_timeout(5000);  // 根据实际情况设置
```

### 6.2 应避免的做法 (DON'T)

❌ **不要在方法中阻塞太久**
```cpp
// 不好：长时间阻塞事件循环
server.register_method("slow", []() -> void {
    std::this_thread::sleep_for(std::chrono::seconds(60));  // 避免
});
```

❌ **不要忽略错误**
```cpp
// 不好：不检查错误
Response resp = client.call("method");
auto result = resp.result();  // 如果是错误响应，会崩溃

// 好：总是检查错误
if (!resp.is_error()) {
    auto result = resp.result();
}
```

❌ **不要在多线程中共享 Client 对象**
```cpp
// 不好：Client 不是线程安全的
Client client("127.0.0.1", 8080);
std::thread t1([&]() { client.call("method1"); });
std::thread t2([&]() { client.call("method2"); });  // 竞态条件

// 好：每个线程使用独立的 Client
std::thread t1([]() {
    Client client("127.0.0.1", 8080);
    client.call("method1");
});
```

❌ **不要注册过多方法到同一个服务器**
```cpp
// 考虑按功能拆分为多个服务
```

## 总结

本教程涵盖了 JsonRPC 的核心功能和使用方法。更多示例请参考 `examples/` 目录，更多架构细节请参考 @ref architecture.md "架构设计文档"。

## 作者

无事情小神仙
