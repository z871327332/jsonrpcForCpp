#include <jsonrpc/jsonrpc.hpp>
#include <chrono>
#include <iostream>
#include <thread>

using namespace jsonrpc;

int main() {
    Client client("127.0.0.1", 9000);  // 故意连接到没有服务的端口
    client.set_timeout(std::chrono::milliseconds(500));

    const int max_attempts = 3;
    for (int attempt = 1; attempt <= max_attempts; ++attempt) {
        try {
            std::cout << "第 " << attempt << " 次尝试调用 ping()" << std::endl;
            client.call<int>("ping");
            std::cout << "调用成功" << std::endl;
            break;
        } catch (const Error& e) {
            std::cerr << "调用失败: " << e.message() << std::endl;
            if (attempt == max_attempts) {
                std::cerr << "达到最大重试次数" << std::endl;
            } else {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    }

    return 0;
}
