#include <jsonrpc/jsonrpc.hpp>
#include <iostream>

using namespace jsonrpc;

int main() {
    Client client("127.0.0.1", 8080);

    try {
        auto result = client.call<int>("divide", 10, 0);
        std::cout << "结果: " << result << std::endl;
    } catch (const Error& e) {
        std::cerr << "捕获到 JSON-RPC 错误: " << e.message()
                  << " (code=" << static_cast<int>(e.code()) << ")" << std::endl;
    }

    try {
        client.call<int>("method_not_exists", 1, 2);
    } catch (const Error& e) {
        std::cerr << "未知方法错误: " << e.message() << std::endl;
    }

    return 0;
}
