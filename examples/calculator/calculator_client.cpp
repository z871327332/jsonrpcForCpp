#include <jsonrpc/jsonrpc.hpp>
#include <iostream>

using namespace jsonrpc;

int main() {
    try {
        Client client("127.0.0.1", 8080);

        auto sum = client.call<int>("add", 10, 20);
        std::cout << "10 + 20 = " << sum << std::endl;

        auto diff = client.call<int>("subtract", 55, 13);
        std::cout << "55 - 13 = " << diff << std::endl;

        auto ratio = client.call<double>("divide", 42, 5);
        std::cout << "42 / 5 = " << ratio << std::endl;

        client.notify("log", "客户端发送的通知");
        std::cout << "通知已发送（无需等待响应）" << std::endl;
    } catch (const Error& e) {
        std::cerr << "RPC 错误: " << e.message() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "客户端异常: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
