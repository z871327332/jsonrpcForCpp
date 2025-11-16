#include <jsonrpc/jsonrpc.hpp>
#include <atomic>
#include <chrono>
#include <iostream>

using namespace jsonrpc;

int main() {
    Client client("127.0.0.1", 8080);
    std::atomic<int> completed_calls{0};

    auto callback = [&](const Response& resp) {
        if (!resp.is_error()) {
            std::cout << "异步结果: " << resp.result().as_int64() << std::endl;
        } else {
            std::cerr << "异步错误: " << resp.error().message() << std::endl;
        }
        ++completed_calls;
    };

    client.async_call("add", callback, 1, 2);
    client.async_call("multiply", callback, 3, 4);
    client.async_call("subtract", callback, 20, 5);

    std::cout << "等待异步调用完成..." << std::endl;
    auto processed = client.run_for(std::chrono::seconds(2));
    std::cout << "run_for() 处理了 " << processed << " 个事件" << std::endl;
    auto drained = client.run_until_idle();
    std::cout << "run_until_idle() 又处理了 " << drained << " 个事件" << std::endl;

    std::cout << "总计完成 " << completed_calls.load() << " 个调用" << std::endl;
    return 0;
}
