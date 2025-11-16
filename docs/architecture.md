# JsonRPC 架构设计文档

本文档详细介绍 JsonRPC 库的系统架构、设计决策和实现细节。

## 第一部分：架构概览

JsonRPC 采用经典的分层架构，从下到上分为三层：

```
┌─────────────────────────────────────┐
│      应用层 (Application Layer)      │
│  - 方法注册 (MethodRegistry)         │
│  - 方法包装 (MethodWrapper)          │
│  - 类型转换 (TypeConverter)          │
├─────────────────────────────────────┤
│      协议层 (Protocol Layer)         │
│  - JSON-RPC 2.0 解析和序列化         │
│  - 请求验证                          │
│  - 批量请求处理                       │
├─────────────────────────────────────┤
│      传输层 (Transport Layer)        │
│  - HTTP/1.1 over TCP                │
│  - Boost.Beast 实现                 │
│  - 连接管理和会话控制                 │
└─────────────────────────────────────┘
```

### 层次职责

**传输层**：负责网络通信，包括 TCP 连接建立、HTTP 协议处理、Keep-Alive 管理。

**协议层**：负责 JSON-RPC 2.0 协议的解析和序列化，验证请求格式，处理批量请求。

**应用层**：负责方法注册、调用分发、参数类型转换、返回值序列化。

## 第二部分：传输层设计

### 2.1 HTTP/1.1 over TCP

JsonRPC 使用 HTTP/1.1 作为传输协议，具有以下优势：

- **广泛兼容**：任何支持 HTTP 的客户端都可以调用
- **防火墙友好**：HTTP 通常不会被防火墙阻止
- **工具丰富**：可以使用 curl、浏览器等工具测试
- **Keep-Alive 支持**：复用 TCP 连接，减少握手开销

### 2.2 Boost.Beast 实现

使用 Boost.Beast 提供高性能的 HTTP 服务器和客户端：

```cpp
// 服务器端：ServerSession
class ServerSession : public std::enable_shared_from_this<ServerSession> {
    boost::beast::tcp_stream stream_;           // TCP 流
    boost::beast::flat_buffer buffer_;          // 读取缓冲区
    boost::beast::http::request<...> req_;      // HTTP 请求
    boost::beast::http::response<...> res_;     // HTTP 响应
    std::shared_ptr<MethodRegistry> registry_;  // 方法注册表

    void do_read();    // 异步读取请求
    void on_read();    // 请求读取完成
    void process();    // 处理请求
    void do_write();   // 异步写入响应
    void on_write();   // 响应写入完成
};
```

### 2.3 连接管理

**服务器端**：
- 每个客户端连接创建一个 `ServerSession` 对象
- 使用 `shared_ptr` 管理会话生命周期
- 支持 Keep-Alive，单个连接可处理多个请求
- 超时控制：30 秒无活动自动关闭

**客户端**：
- `Client` 对象维护一个 `ClientSession`
- 复用 TCP 连接发送多个请求
- 支持超时设置（默认 2 分钟）

### 2.4 异步 I/O

传输层完全基于 Boost.Asio 的异步 I/O：

```cpp
// 异步读取示例
boost::beast::http::async_read(
    stream_,
    buffer_,
    req_,
    [self = shared_from_this()](boost::beast::error_code ec, std::size_t) {
        self->on_read(ec);
    }
);
```

优势：
- 单线程可处理数千并发连接
- 非阻塞 I/O，高效利用 CPU
- 事件驱动，低延迟

## 第三部分：协议层设计

### 3.1 JSON-RPC 2.0 协议

协议层实现了完整的 JSON-RPC 2.0 规范：

**请求格式**：
```json
{
    "jsonrpc": "2.0",
    "method": "add",
    "params": [10, 20],
    "id": 1
}
```

**响应格式**：
```json
{
    "jsonrpc": "2.0",
    "result": 30,
    "id": 1
}
```

**错误响应格式**：
```json
{
    "jsonrpc": "2.0",
    "error": {
        "code": -32602,
        "message": "Invalid params"
    },
    "id": 1
}
```

### 3.2 Protocol 类

`Protocol` 类提供静态方法进行协议处理：

```cpp
class Protocol {
public:
    // 服务器端方法
    static std::vector<Request> parse_request(const std::string& json_str);
    static std::string serialize_response(const Response& response);
    static std::string serialize_batch_response(const std::vector<Response>& responses);

    // 客户端方法
    static std::string serialize_request(const Request& request);
    static Response parse_response(const std::string& json_str);
    static std::vector<Response> parse_batch_response(const std::string& json_str);
};
```

### 3.3 请求验证

协议层会验证请求的有效性：

1. **JSON 格式验证**：确保是有效的 JSON
2. **协议版本验证**：检查 `jsonrpc` 字段是否为 "2.0"
3. **必需字段验证**：检查 `method` 字段是否存在
4. **类型验证**：确保各字段类型正确

验证失败会返回相应的错误码。

### 3.4 批量请求处理

批量请求格式：
```json
[
    {"jsonrpc": "2.0", "method": "add", "params": [1, 2], "id": 1},
    {"jsonrpc": "2.0", "method": "subtract", "params": [5, 3], "id": 2}
]
```

批量响应格式：
```json
[
    {"jsonrpc": "2.0", "result": 3, "id": 1},
    {"jsonrpc": "2.0", "result": 2, "id": 2}
]
```

## 第四部分：应用层设计

### 4.1 MethodRegistry - 方法注册表

`MethodRegistry` 管理所有已注册的 RPC 方法：

```cpp
class MethodRegistry {
public:
    template<typename Func>
    void register_method(const std::string& name, Func&& func);

    Response invoke(const Request& request);
    std::vector<Response> invoke_batch(const std::vector<Request>& requests);

private:
    std::map<std::string, std::shared_ptr<MethodWrapperBase>> methods_;
    std::mutex mutex_;  // 线程安全保护
};
```

**线程安全**：使用 `std::mutex` 保护 `methods_` 的并发访问。

### 4.2 MethodWrapper - 方法包装

`MethodWrapper` 负责：
1. 从 JSON 参数提取 C++ 类型参数
2. 调用实际的方法实现
3. 将返回值转换为 JSON

```cpp
class MethodWrapperBase {
public:
    virtual ~MethodWrapperBase() = default;
    virtual boost::json::value invoke(const boost::json::value& params) = 0;
};

template<typename Func>
class MethodWrapperImpl : public MethodWrapperBase {
    Func func_;

public:
    boost::json::value invoke(const boost::json::value& params) override {
        // 1. 提取参数
        auto args_tuple = extract_args<Args...>(params);

        // 2. 调用函数
        R result = call_with_tuple(func_, std::move(args_tuple));

        // 3. 转换返回值
        return json_converter<R>::to_json(result);
    }
};
```

### 4.3 TypeConverter - 类型转换

`json_converter` 模板提供类型转换功能：

```cpp
template<typename T>
struct json_converter {
    static boost::json::value to_json(const T& value);
    static T from_json(const boost::json::value& v);
};
```

已实现的特化：
- 基本类型：`int`, `double`, `bool`, `std::string`
- 容器类型：`std::vector<T>`, `std::map<std::string, T>`
- 特殊类型：`void`（用于无返回值方法）

### 4.4 FunctionTraits - 函数特征提取

使用模板元编程提取函数签名信息：

```cpp
template<typename Func>
struct function_traits {
    typedef ... return_type;       // 返回类型
    typedef ... args_tuple;        // 参数类型 tuple
    static constexpr size_t arity; // 参数数量
};
```

支持的函数类型：
- 普通函数指针
- Lambda 表达式
- `std::function`
- 成员函数指针

## 第五部分：线程模型

### 5.1 单线程异步事件循环

**服务器端**：
```cpp
void Server::run() {
    io_context_.run();  // 阻塞运行事件循环
}

void Server::start() {
    worker_thread_ = std::make_unique<std::thread>([this]() {
        io_context_.run();  // 在独立线程运行事件循环
    });
}
```

事件循环负责：
- 接受新连接
- 读取 HTTP 请求
- 写入 HTTP 响应
- 超时管理

**优势**：
- 无锁设计，避免竞态条件
- 高效的 CPU 利用率
- 支持数千并发连接

### 5.2 批量请求并行处理

虽然服务器主循环是单线程，但批量请求中的方法调用是并行执行的：

```cpp
std::vector<Response> MethodRegistry::invoke_batch(const std::vector<Request>& requests) {
    std::vector<std::future<IndexedResponse>> futures;

    // 并行发起所有请求
    for (size_t i = 0; i < requests.size(); ++i) {
        futures.emplace_back(
            std::async(std::launch::async, [this, request = requests[i], i]() {
                return IndexedResponse(i, invoke(request));
            })
        );
    }

    // 收集结果并排序
    std::vector<IndexedResponse> indexed_responses;
    for (auto& fut : futures) {
        indexed_responses.push_back(fut.get());
    }

    std::sort(indexed_responses.begin(), indexed_responses.end(),
        [](const auto& a, const auto& b) { return a.first < b.first; });

    // 提取响应
    std::vector<Response> responses;
    for (auto& item : indexed_responses) {
        responses.push_back(std::move(item.second));
    }

    return responses;
}
```

**设计原理**：
- 主事件循环保持单线程，负责 I/O
- 计算密集的方法调用使用 `std::async` 并行执行
- 通过索引保持响应顺序
- 兼顾了性能和简洁性

### 5.3 线程安全保证

- **MethodRegistry**：使用 `std::mutex` 保护方法表
- **ServerSession**：每个会话独立，无共享状态
- **Client**：单个 `Client` 对象不是线程安全的，多线程环境需使用独立的 `Client` 对象

## 第六部分：内存管理

### 6.1 RAII 资源管理

所有资源使用 RAII 原则管理：

```cpp
class Server::Impl {
    boost::asio::io_context io_context_;           // 自动析构
    boost::asio::ip::tcp::acceptor acceptor_;      // 自动关闭
    std::unique_ptr<std::thread> worker_thread_;   // 自动 join
};
```

### 6.2 智能指针使用

**shared_ptr**：
- `ServerSession` 使用 `shared_ptr` 管理生命周期
- 异步操作捕获 `shared_ptr`，确保对象在异步完成前不被销毁

```cpp
auto self = shared_from_this();
boost::beast::http::async_read(..., [self](...) {
    // self 保证对象存活
});
```

**unique_ptr**：
- Pimpl 实现使用 `unique_ptr`
- 线程对象使用 `unique_ptr`

### 6.3 会话生命周期

```
客户端连接 → 创建 ServerSession (shared_ptr)
           ↓
         do_read() → 捕获 shared_ptr
           ↓
        on_read() → process() → do_write()
           ↓
       on_write() → 检查 Keep-Alive
           ↓
      Keep-Alive?
       Yes → do_read() (循环)
       No  → 连接关闭，shared_ptr 引用计数归零，对象销毁
```

## 第七部分：数据流

### 7.1 请求处理流程

```
客户端发送 HTTP POST
    ↓
ServerSession::do_read()
    ↓
验证 HTTP 方法和 Content-Type
    ↓
Protocol::parse_request()
    ↓
MethodRegistry::invoke_batch()
    ↓
并行执行各个方法调用
    ↓
收集响应并排序
    ↓
Protocol::serialize_batch_response()
    ↓
ServerSession::do_write()
    ↓
发送 HTTP 响应给客户端
```

### 7.2 错误处理路径

```
异常发生
    ↓
MethodWrapper 捕获
    ↓
是 jsonrpc::Error?
  Yes → 直接使用
  No  → 转换为 InternalError
    ↓
包装为 Response 对象
    ↓
序列化为 JSON 错误响应
    ↓
返回给客户端
```

## 第八部分：设计决策

### 8.1 为什么选择 Boost.Beast

**考虑的备选方案**：
- libcurl：主要用于客户端，服务器支持不足
- libevent + evhttp：C 风格 API，类型不安全
- C++的其他 HTTP 库：依赖太多或不够成熟

**选择 Boost.Beast 的原因**：
- 现代 C++ 设计，类型安全
- 与 Boost.Asio 无缝集成
- Header-only，无需额外编译
- 性能优秀，内存效率高
- 文档完善，社区活跃

### 8.2 为什么使用单线程事件循环

**优势**：
- 无锁设计，避免死锁和竞态条件
- 代码简单，易于维护
- 高效的 I/O 多路复用
- 适合 I/O 密集型工作负载

**适用场景**：
- RPC 调用通常是 I/O 密集型
- 大部分时间在等待网络和远程方法执行
- 单线程足以处理数千并发连接

### 8.3 为什么批量请求使用并行处理

批量请求中的方法调用可能是 CPU 密集型的，顺序执行会浪费多核 CPU 资源。使用 `std::async` 并行执行：

- 充分利用多核 CPU
- 显著提高批量请求的吞吐量
- 实现简单，使用标准库功能
- 保持响应顺序，符合 JSON-RPC 2.0 规范

### 8.4 Header-only vs 编译库

**提供两种模式**：

**Header-only（默认）**：
- 无需编译和安装
- 集成简单，只需 include
- 适合小型项目和快速原型

**编译库**：
- 减少编译时间（大型项目）
- 减小可执行文件大小
- 便于分发和版本管理

## 第九部分：性能特性

### 9.1 低延迟设计

- 异步 I/O，非阻塞操作
- 零拷贝设计（Boost.Beast）
- 编译时类型推导，运行时开销小
- 本地调用延迟 < 1ms

### 9.2 高吞吐量优化

- HTTP Keep-Alive 复用连接
- 批量请求并行处理
- 单线程事件循环支持数千并发
- 智能指针池化（避免频繁分配）

### 9.3 内存效率

- RAII 自动资源管理
- 智能指针，零内存泄漏
- Boost.Beast flat_buffer 减少内存分配
- 按需创建会话对象

### 9.4 可扩展性

- 单个服务器进程可处理数千并发连接
- 可通过负载均衡水平扩展
- 无状态设计，易于分布式部署

## 第十部分：未来优化方向

### 可能的改进

1. **连接池**：客户端使用连接池复用连接
2. **线程池**：替代 `std::async`，避免频繁创建线程
3. **压缩**：支持 gzip 压缩减少带宽
4. **TLS 支持**：HTTPS 传输加密
5. **WebSocket**：支持双向通信
6. **性能监控**：内置性能指标收集

## 总结

JsonRPC 的架构设计遵循以下原则：

- **分层清晰**：传输、协议、应用三层职责明确
- **高性能**：异步 I/O + 批量并行，充分利用硬件资源
- **易用性**：简洁的 API，自动类型转换
- **可靠性**：RAII 资源管理，完善的错误处理
- **可维护性**：现代 C++ 设计，代码清晰

这种设计使得 JsonRPC 既保持了高性能，又易于使用和维护。

## 作者

无事情小神仙
