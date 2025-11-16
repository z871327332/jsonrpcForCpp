# JsonRPC - 高性能 C++ JSON-RPC 2.0 库

## 项目简介

JsonRPC 是一个基于 Boost.JSON 和 Boost.Beast 的高性能、易用的 C++ JSON-RPC 2.0 库。它提供完整的服务器和客户端实现，支持同步和异步调用，具有自动类型转换、批量请求处理等强大特性。

本库专为需要高性能 RPC 通信的 C++ 应用程序设计，采用现代 C++11 标准编写，支持跨平台部署（Linux/Windows），并提供 Header-only 和编译库两种使用模式。

## 主要特性

### 完整的协议支持

- **JSON-RPC 2.0 标准**：完全符合 JSON-RPC 2.0 规范
- **请求/响应模式**：支持标准的请求-响应通信
- **批量请求**：单次 HTTP 请求可包含多个 RPC 调用，支持真正的并行处理
- **通知机制**：支持不需要响应的单向通知
- **标准错误处理**：实现了完整的 JSON-RPC 2.0 错误代码体系

### 高性能设计

- **异步 I/O**：基于 Boost.Asio 的单线程异步事件循环
- **批量并行处理**：批量请求中的多个调用使用 std::async 实现真正的并行执行
- **HTTP Keep-Alive**：支持持久连接，减少连接建立开销
- **低延迟**：本地调用延迟通常低于 1ms
- **高吞吐**：单线程可处理数千并发连接

### 易于使用

- **简洁的 API**：服务器端只需 3 行代码即可启动，客户端调用如同本地函数
- **自动类型转换**：支持 C++ 基本类型、容器类型和自定义类型的自动序列化/反序列化
- **Header-only 模式**：默认为 Header-only，无需编译即可使用
- **丰富的示例**：提供 7 个完整的示例程序，涵盖各种使用场景

### 强大的类型系统

- **基本类型**：int, double, bool, string 自动转换
- **容器类型**：std::vector, std::map 原生支持
- **嵌套类型**：支持任意深度的嵌套容器
- **自定义类型**：通过特化 json_converter 轻松扩展

## 快速开始

### 服务器示例

```cpp
#include <jsonrpc/jsonrpc.hpp>

using namespace jsonrpc;

int main() {
    // 创建服务器
    Server server("127.0.0.1", 8080);

    // 注册方法
    server.register_method("add", [](int a, int b) -> int {
        return a + b;
    });

    // 启动服务器（阻塞运行）
    server.run();

    return 0;
}
```

### 客户端示例

```cpp
#include <jsonrpc/jsonrpc.hpp>
#include <iostream>

using namespace jsonrpc;

int main() {
    // 创建客户端
    Client client("127.0.0.1", 8080);

    // 同步调用
    Response resp = client.call("add", 10, 20);

    if (!resp.is_error()) {
        std::cout << "结果: " << resp.result().as_int64() << std::endl;
    } else {
        std::cerr << "错误: " << resp.error().message() << std::endl;
    }

    return 0;
}
```

## 系统架构

JsonRPC 采用分层架构设计，从下到上分为三层：

### 传输层（Transport Layer）

- **协议**：基于 HTTP/1.1 over TCP
- **实现**：使用 Boost.Beast 提供高性能 HTTP 服务器和客户端
- **特性**：Keep-Alive 持久连接、超时控制、会话管理

### 协议层（Protocol Layer）

- **解析**：JSON-RPC 2.0 请求解析和验证
- **序列化**：响应和错误的 JSON 序列化
- **批量处理**：批量请求的拆分和聚合

### 应用层（Application Layer）

- **方法注册**：类型安全的方法注册机制
- **方法调用**：自动参数提取和返回值转换
- **类型转换**：编译时类型推导和运行时转换
- **错误处理**：异常捕获和错误对象生成

## 文档导航

### 使用指南

- @ref tutorial.md "使用教程" - 详细的功能介绍和使用示例
- @ref architecture.md "架构设计" - 系统架构和设计决策
- [示例程序](../examples/README.md) - 7 个完整的示例程序

### API 参考

- @ref jsonrpc::Server "Server 类" - 服务器接口
- @ref jsonrpc::Client "Client 类" - 客户端接口
- @ref jsonrpc::Request "Request 类" - 请求对象
- @ref jsonrpc::Response "Response 类" - 响应对象
- @ref jsonrpc::Error "Error 类" - 错误对象

### 核心组件

- @ref jsonrpc::detail::MethodRegistry "MethodRegistry" - 方法注册表
- @ref jsonrpc::detail::Protocol "Protocol" - 协议处理
- @ref jsonrpc::detail::json_converter "TypeConverter" - 类型转换器

## 编译和安装

### 系统要求

- **C++ 编译器**：GCC 4.8+, Clang 3.4+, MSVC 2015+
- **C++ 标准**：C++11 或更高
- **Boost 版本**：1.83+ (需要 JSON, Beast, Asio, System 组件)
- **CMake**：3.10+

### Header-Only 模式（推荐）

```bash
git clone https://github.com/z871327332/jsonrpcForCpp.git
cd jsonrpcForCpp
mkdir build && cd build
cmake ..
make
```

### 编译为库

```bash
git clone https://github.com/z871327332/jsonrpcForCpp.git
cd jsonrpcForCpp
mkdir build && cd build
cmake -DJSONRPC_HEADER_ONLY=OFF ..
make
sudo make install
```

### 运行测试

```bash
cd build
make test
# 或直接运行测试程序
./tests/jsonrpc_tests
```

### 生成文档

```bash
# 确保已安装 Doxygen
make doc
# 文档生成在 docs/html/index.html
```

## 集成到项目

### Header-Only 模式

在你的 `CMakeLists.txt` 中：

```cmake
find_package(Boost 1.83 REQUIRED COMPONENTS json system)
include_directories(/path/to/jsonrpc/include)
target_link_libraries(your_target ${Boost_LIBRARIES} pthread)
```

在代码中：

```cpp
#include <jsonrpc/jsonrpc.hpp>
```

### 编译库模式

```cmake
find_package(JsonRPC REQUIRED)
target_link_libraries(your_target jsonrpc)
```

## 性能特性

- **低延迟**：基于异步 I/O，本地调用延迟 < 1ms
- **高吞吐**：批量请求并行处理，显著提升吞吐量
- **可扩展**：单线程异步事件循环，支持数千并发连接
- **内存高效**：RAII 资源管理，智能指针生命周期控制，零内存泄漏

## 许可证

本项目采用 MIT 许可证。详见 [LICENSE](../LICENSE) 文件。

## 作者

无事情小神仙

## 贡献

欢迎贡献代码、报告问题或提出建议！请访问项目的 [GitHub 仓库](https://github.com/z871327332/jsonrpcForCpp)。

## 相关链接

- [JSON-RPC 2.0 规范](https://www.jsonrpc.org/specification)
- [Boost 文档](https://www.boost.org/doc/)
- [Boost.Beast](https://www.boost.org/doc/libs/release/libs/beast/)
- [Boost.JSON](https://www.boost.org/doc/libs/release/libs/json/)

---

**开始使用 JsonRPC，享受高性能的 RPC 通信体验！**
