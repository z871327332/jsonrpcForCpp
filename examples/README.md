# 示例程序

每个示例都可以在构建目录中通过 `make <target>` 生成，亦可手动运行 `cmake --build . --target <target>`。

| 目标 | 说明 | 入口 |
| --- | --- | --- |
| `calculator_server` | 启动基础计算器服务，演示方法注册、批量线程池配置、日志回调及错误处理 | `examples/calculator/calculator_server.cpp` |
| `calculator_client` | 调用计算器服务，演示同步调用与通知 | `examples/calculator/calculator_client.cpp` |
| `async_client` | 并发发起多个异步请求并通过 `Client::run()` 等待结果 | `examples/async_client/main.cpp` |
| `batch_request` | 构造 `Request` 列表并一次性发送，实现批量请求 | `examples/batch_request/main.cpp` |
| `type_conversion` | 自定义 `json_converter`，展示复杂类型序列化 | `examples/type_conversion/main.cpp` |
| `error_handling` | 演示如何捕获 `jsonrpc::Error` 并打印错误信息 | `examples/error_handling/main.cpp` |
| `timeout_retry` | 演示超时设置与简单的重试策略 | `examples/timeout_retry/main.cpp` |

运行示例前请先在服务器端启动相应服务（例如 `calculator_server`）。客户端示例默认连接 `127.0.0.1:8080`。***
> 配置批量线程池（`server.set_batch_concurrency()`）务必在调用 `run()`/`start()` 之前完成，或在 `stop()` 后再调整；通知请求会在客户端侧立即返回（通常 <200ms），服务端执行后不返回结果。
