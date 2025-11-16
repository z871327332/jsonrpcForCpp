# JsonRPC - 高性能 C++ JSON-RPC 2.0 库

[![C++11](https://img.shields.io/badge/C%2B%2B-11-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B11)
[![Boost](https://img.shields.io/badge/Boost-1.83%2B-orange.svg)](https://www.boost.org/)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)

一个基于 Boost.JSON 的高性能、易用的 C++ JSON-RPC 2.0 库。提供完整的服务器和客户端实现，支持同步和异步调用，具有自动类型转换、批量请求处理等强大特性。

## 主要特性

### 核心功能

- ✅ **完整的 JSON-RPC 2.0 协议支持**
  - 请求/响应处理
  - 批量请求
  - 通知（不等待响应）
  - 标准错误代码

- ✅ **高性能设计**
  - 基于 Boost.Asio 的异步 I/O
  - HTTP Keep-Alive 支持
  - 批量请求并行处理
  - 批量线程池可配置（`Server::set_batch_concurrency`）
  - 单线程异步事件循环

- ✅ **易于使用**
  - 简洁的 API 设计
  - 自动参数类型转换
  - Header-only 或编译库两种模式
  - 丰富的示例程序
  - 服务器可多次 start/stop，支持批量线程池调优
  - 客户端/服务器均提供可选日志回调

- ✅ **类型系统**
  - 支持基本类型（int, double, bool, string）
  - 支持容器类型（vector, map）
  - 支持嵌套类型
  - 自动序列化/反序列化

- ✅ **平台支持**
  - 跨平台（Linux, Windows）
  - C++11 标准
  - CMake 构建系统

## 系统要求

- **C++ 编译器**: GCC 4.8+, Clang 3.4+, MSVC 2015+
- **C++ 标准**: C++11 或更高
- **Boost 版本**: 1.83+（需要 JSON, Beast, Asio, System 组件）
- **CMake**: 3.10+

## 快速开始

### 服务器示例

```cpp
#include <jsonrpc/jsonrpc.hpp>

using namespace jsonrpc;

int main() {
    // 创建服务器
    Server server("127.0.0.1", 8080);
    server.set_batch_concurrency(4); // 自定义批量请求线程池
    server.set_logger([](const std::string& msg) {
        std::cout << "[SERVER] " << msg << std::endl;
    });

    // 注册方法
    server.register_method("add", [](int a, int b) -> int {
        return a + b;
    });

    // 启动服务器（阻塞运行）
    server.run();

    // 若采用 server.start()（后台线程），可以 stop 后再次 start 以重启服务：
    // server.start();
    // ...
    // server.stop();
    // server.start();
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
    client.set_logger([](const std::string& msg) {
        std::cout << "[CLIENT] " << msg << std::endl;
    });

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

### 客户端事件循环辅助

异步场景下，可使用以下方法更灵活地驱动事件循环：

- `run_for(std::chrono::steady_clock::duration duration)`：在限定时间内运行事件循环，适合需要定时返回主循环的场景。
- `run_until_idle()`：处理所有已就绪的事件后立即返回，不会等待新的网络事件，适合在主循环中定期冲刷未处理的回调。

### 日志回调

Client 与 Server 均提供日志回调，便于在调试阶段捕获网络错误或无效请求：

```cpp
jsonrpc::Client client("127.0.0.1", 8080);
client.set_logger([](const std::string& msg) {
    std::cout << "[CLIENT] " << msg << std::endl;
});

jsonrpc::Server server(8080);
server.set_logger([](const std::string& msg) {
    std::cout << "[SERVER] " << msg << std::endl;
});
```

> 回调在 I/O 线程中执行，请确保输出逻辑是线程安全的。

## 编译和安装

### 克隆仓库

```bash
git clone https://github.com/z871327332/jsonrpcForCpp.git
cd jsonrpcForCpp
```

### 准备 Boost 依赖

本项目支持两种方式使用 Boost：

#### 方式 1：使用本地 Boost（推荐，无需安装系统 Boost）

运行脚本自动下载 Boost 1.83.0 到 `third_party/boost/`：

**Linux / macOS**:
```bash
bash scripts/download_boost.sh
```

**Windows**:
```batch
scripts\download_boost.bat
```

下载完成后，CMake 会自动检测并使用本地 Boost，无需额外配置。

#### 方式 2：使用系统 Boost

如果系统已安装 Boost 1.83+，可跳过下载脚本。CMake 会在本地 Boost 不存在时自动回退到系统 Boost。

**Ubuntu/Debian**:
```bash
sudo apt-get install libboost1.83-all-dev
```

**macOS (Homebrew)**:
```bash
brew install boost
```

**Windows**:
- 从 [Boost 官网](https://www.boost.org/) 下载预编译包
- 或使用 vcpkg: `vcpkg install boost`

### 编译（Header-Only 模式，默认）

```bash
mkdir build
cd build
cmake ..
make
```

### 编译为静态/动态库

```bash
mkdir build
cd build
cmake -DJSONRPC_HEADER_ONLY=OFF ..
make
```

### 运行测试

```bash
make test
# 或
./tests/jsonrpc_tests
```

### 生成文档

```bash
make doc
```

生成的文档位于 `docs/html/index.html`。

## 使用方法

### 集成到项目（Header-Only 模式）

在你的 `CMakeLists.txt` 中：

```cmake
# 查找 Boost
find_package(Boost 1.83 REQUIRED COMPONENTS json system)

# 包含 JsonRPC 头文件
include_directories(/path/to/jsonrpc/include)

# 链接 Boost 库
target_link_libraries(your_target ${Boost_LIBRARIES} pthread)
```

在代码中：

```cpp
#include <jsonrpc/jsonrpc.hpp>
```

### 集成到项目（编译库模式）

```cmake
find_package(JsonRPC REQUIRED)
target_link_libraries(your_target jsonrpc)
```

### 批量请求线程池调优

服务器默认使用 `std::thread::hardware_concurrency()`（至少 2）个线程来并行处理批量请求。你可以在服务启动时调用：

```cpp
jsonrpc::Server server(8080);
server.set_batch_concurrency(8);  // 根据负载调整线程数（最少 1）
```

变更会重建内部线程池，请在服务开始处理正式流量之前设置。

> ⚠️ 注意：`set_batch_concurrency()` 仅可在服务器尚未运行或调用 `stop()` 之后执行；若在运行状态下调用会抛出 `std::logic_error`。需要在运行时调整时，请先停止服务、调整并重新启动。
>
> 可通过 `server.is_running()` 判断当前运行状态，从而避免重复调用 `run()` / `start()`。

## 示例程序

项目提供了 7 个完整的示例程序，位于 `examples/` 目录：

| 示例程序 | 说明 |
|---------|------|
| **calculator_server** | 计算器服务器，演示方法注册、错误处理 |
| **calculator_client** | 计算器客户端，演示同步调用、通知 |
| **async_client** | 异步调用示例，演示并发请求 |
| **batch_request** | 批量请求示例，演示批量处理性能 |
| **type_conversion** | 类型转换示例，演示各种 C++ 类型 |
| **error_handling** | 错误处理示例，演示错误码和异常 |
| **timeout_retry** | 超时和重试示例，演示超时设置和重试逻辑 |

详见 [examples/README.md](examples/README.md)。

### 编译示例程序

```bash
cd build
make
```

示例程序位于 `build/examples/` 目录。

### 运行示例

```bash
# 启动计算器服务器
./examples/calculator_server

# 在另一个终端运行客户端
./examples/calculator_client
```

## 文档

### 在线文档

- **主页**: 运行 `make doc` 后打开 `docs/html/index.html`
- **使用教程**: [docs/tutorial.md](docs/tutorial.md)
- **架构设计**: [docs/architecture.md](docs/architecture.md)

### API 参考

- [Server 类](docs/html/classjsonrpc_1_1_server.html)
- [Client 类](docs/html/classjsonrpc_1_1_client.html)
- [Request 类](docs/html/classjsonrpc_1_1_request.html)
- [Response 类](docs/html/classjsonrpc_1_1_response.html)
- [Error 类](docs/html/classjsonrpc_1_1_error.html)

## 测试

JsonRPC 包含完整的单元测试和集成测试：

- **单元测试**: 94 个测试用例
- **集成测试**: 15 个端到端测试
- **测试框架**: Google Test

### 运行所有测试

```bash
cd build
make test
```

### 运行特定测试

```bash
cd build
./tests/jsonrpc_tests --gtest_filter=ServerTest.*
```

### 测试覆盖

- 类型系统（Request, Response, Error）
- 协议层（JSON-RPC 解析/序列化）
- 类型转换（基本类型、容器、嵌套类型）
- 服务器（方法注册、请求处理、错误处理）
- 客户端（同步/异步调用、超时、批量请求）
- 集成测试（端到端、并发、性能）

## 核心 API

### 服务器端

```cpp
// 创建服务器
Server server("127.0.0.1", 8080);

// 注册方法（支持任意参数类型）
server.register_method("add", [](int a, int b) -> int {
    return a + b;
});

// 注册方法（带错误处理）
server.register_method("divide", [](int a, int b) -> double {
    if (b == 0) {
        throw Error(ErrorCode::InvalidParams, "除数不能为零");
    }
    return static_cast<double>(a) / b;
});

// 启动服务器
server.start();  // 异步启动
// 或
server.run();    // 阻塞运行
```

### 客户端

```cpp
// 创建客户端
Client client("127.0.0.1", 8080);

// 设置超时
client.set_timeout(5000);  // 5 秒

// 同步调用
Response resp = client.call("add", 10, 20);
if (!resp.is_error()) {
    int result = resp.result().as_int64();
}

// 异步调用
client.async_call("add", [](const Response& resp) {
    if (!resp.is_error()) {
        std::cout << "结果: " << resp.result().as_int64() << std::endl;
    }
}, 10, 20);

// 发送通知（不等待响应）
client.notify("log", "消息");
```

## 性能特性

- **低延迟**: 基于异步 I/O，本地调用延迟 < 1ms
- **高吞吐**: 支持批量请求，减少网络开销
- **可扩展**: 异步事件循环，支持大量并发连接
- **内存效率**: RAII 资源管理，零内存泄漏

## 许可证

本项目采用 MIT 许可证，详见 [LICENSE](LICENSE) 文件。

## 作者

无事情小神仙

## 贡献

欢迎贡献！请遵循以下步骤：

1. Fork 本仓库
2. 创建特性分支 (`git checkout -b feature/amazing-feature`)
3. 提交更改 (`git commit -m 'Add amazing feature'`)
4. 推送到分支 (`git push origin feature/amazing-feature`)
5. 开启 Pull Request

## 相关链接

- [JSON-RPC 2.0 规范](https://www.jsonrpc.org/specification)
- [Boost 文档](https://www.boost.org/doc/)
- [Boost.Beast](https://www.boost.org/doc/libs/release/libs/beast/)
- [Boost.JSON](https://www.boost.org/doc/libs/release/libs/json/)

## 问题反馈

如有问题或建议，请：

- 提交 [Issue](https://github.com/z871327332/jsonrpcForCpp/issues)
- 发送邮件到维护者

## 更新日志

### v1.0.0 (2025-11-16)

**初始版本发布**

- ✅ 完整的 JSON-RPC 2.0 协议支持
- ✅ HTTP/1.1 传输协议
- ✅ 同步和异步调用接口
- ✅ 自动类型转换系统
- ✅ 批量请求支持
- ✅ 完整的单元测试和集成测试
- ✅ 7 个示例程序
- ✅ 详细的文档和 API 参考
- ✅ 跨平台支持（Linux, Windows）
- ✅ Header-only 和编译库两种模式

---

**感谢使用 JsonRPC！** 🚀
