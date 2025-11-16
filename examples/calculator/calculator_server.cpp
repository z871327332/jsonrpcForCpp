#include <jsonrpc/jsonrpc.hpp>
#include <iostream>

using namespace jsonrpc;

int main() {
    try {
        Server server(8080);
        server.set_batch_concurrency(4);
        server.set_logger([](const std::string& msg) {
            std::cout << "[SERVER] " << msg << std::endl;
        });

        server.register_method("add", [](int a, int b) -> int {
            return a + b;
        });
        server.register_method("subtract", [](int a, int b) -> int {
            return a - b;
        });
        server.register_method("multiply", [](int a, int b) -> int {
            return a * b;
        });

        server.register_method("divide", [](int a, int b) -> double {
            if (b == 0) {
                throw Error(ErrorCode::InvalidParams, "除数不能为 0");
            }
            return static_cast<double>(a) / static_cast<double>(b);
        });

        std::cout << "Calculator server listening on http://127.0.0.1:8080" << std::endl;
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "服务器异常: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
