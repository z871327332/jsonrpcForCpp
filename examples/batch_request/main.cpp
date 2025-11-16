#include <jsonrpc/jsonrpc.hpp>
#include <iostream>
#include <vector>

using namespace jsonrpc;

int main() {
    Client client("127.0.0.1", 8080);

    std::vector<Request> requests;
    requests.emplace_back(
        "add",
        boost::json::array{1, 2},
        boost::json::value(1)
    );
    requests.emplace_back(
        "multiply",
        boost::json::array{3, 5},
        boost::json::value(2)
    );
    requests.emplace_back(
        "subtract",
        boost::json::array{20, 4},
        boost::json::value(3)
    );

    try {
        auto responses = client.call_batch(requests);
        for (const auto& resp : responses) {
            if (resp.is_error()) {
                std::cerr << "请求 " << resp.id() << " 失败: "
                          << resp.error().message() << std::endl;
            } else {
                std::cout << "请求 " << resp.id() << " 的结果: "
                          << boost::json::serialize(resp.result()) << std::endl;
            }
        }
    } catch (const Error& e) {
        std::cerr << "批量调用失败: " << e.message() << std::endl;
        return 1;
    }

    return 0;
}
