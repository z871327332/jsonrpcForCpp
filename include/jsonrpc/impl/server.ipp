#pragma once

#include <jsonrpc/server.hpp>
#include <jsonrpc/detail/method_registry.hpp>
#include <jsonrpc/detail/server_session.hpp>
#include <boost/asio.hpp>
#include <memory>
#include <thread>
#include <atomic>
#include <string>

namespace jsonrpc {

// ============================================================================
// Server::Impl 类（Pimpl 实现）
// ============================================================================

class Server::Impl {
public:
    /**
     * @brief 构造 Impl
     * @param port 监听端口
     * @param address 监听地址
     */
    Impl(unsigned short port, const std::string& address)
        : io_context_()
        , acceptor_(io_context_)
        , registry_(std::make_shared<detail::MethodRegistry>())
        , running_(false)
        , endpoint_(boost::asio::ip::tcp::endpoint(
            boost::asio::ip::make_address(address),
            port
        ))
        , acceptor_ready_(false)
    {
        prepare_acceptor();
    }

    /**
     * @brief 获取 io_context
     */
    boost::asio::io_context& get_io_context() {
        return io_context_;
    }

    /**
     * @brief 获取方法注册表
     */
    std::shared_ptr<detail::MethodRegistry> get_registry() {
        return registry_;
    }

    void set_logger(std::function<void(const std::string&)> logger) {
        logger_ = std::move(logger);
    }

    void log(const std::string& message) {
        if (logger_) {
            logger_(message);
        }
    }

    /**
     * @brief 尝试进入运行状态
     */
    bool try_enter_running() {
        bool expected = false;
        return running_.compare_exchange_strong(expected, true);
    }

    /**
     * @brief 离开运行状态
     */
    void leave_running() {
        running_.store(false);
    }

    /**
     * @brief 查询运行状态
     */
    bool is_running() const {
        return running_.load();
    }

    /**
     * @brief 开始异步接受连接
     */
    void do_accept() {
        acceptor_.async_accept(
            [this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
                on_accept(ec, std::move(socket));
            }
        );
    }

    /**
     * @brief 异步启动（非阻塞）
     */
    void start_async() {
        if (!try_enter_running()) {
            throw std::logic_error("Server is already running");
        }

        prepare_acceptor();

        // 开始接受连接
        do_accept();

        // 创建工作线程
        worker_thread_.reset(new std::thread([this]() {
            io_context_.run();
            io_context_.restart();
            leave_running();
        }));
    }

    /**
     * @brief 停止服务器
     */
    void stop() {
        // 设置运行标志为 false
        if (!running_.exchange(false)) {
            return;  // 未运行，直接返回
        }

        // 关闭 acceptor（停止接受新连接）
        teardown_acceptor();

        // 停止 io_context
        io_context_.stop();

        // 等待工作线程结束
        if (worker_thread_ && worker_thread_->joinable()) {
            worker_thread_->join();
        }
        worker_thread_.reset();
        io_context_.restart();
    }

    void prepare_acceptor() {
        if (acceptor_ready_) {
            return;
        }

        acceptor_.open(endpoint_.protocol());
        acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
        acceptor_.bind(endpoint_);
        acceptor_.listen();
        acceptor_ready_ = true;
    }

    void teardown_acceptor() {
        if (!acceptor_ready_) {
            return;
        }
        boost::system::error_code ec;
        acceptor_.close(ec);
        acceptor_ready_ = false;
    }

private:
    /**
     * @brief 接受连接完成回调
     * @param ec 错误码
     * @param socket 新连接的 socket
     */
    void on_accept(boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
        if (ec) {
            // 如果是 operation_aborted，说明 acceptor 已关闭
            if (ec == boost::asio::error::operation_aborted) {
                return;  // 停止接受循环
            }
            // 其他错误记录并继续
            log(std::string("接受连接失败: ") + ec.message());
        } else {
            // 创建会话并启动
            std::make_shared<detail::ServerSession>(
                std::move(socket),
                registry_,
                logger_
            )->start();
        }

        // 继续接受下一个连接
        do_accept();
    }

    boost::asio::io_context io_context_;                        ///< I/O 上下文
    boost::asio::ip::tcp::acceptor acceptor_;                   ///< TCP 接受器
    std::shared_ptr<detail::MethodRegistry> registry_;          ///< 方法注册表
    std::unique_ptr<std::thread> worker_thread_;                ///< 工作线程
    std::atomic<bool> running_;                                 ///< 运行状态标志
    boost::asio::ip::tcp::endpoint endpoint_;                   ///< 监听地址
    bool acceptor_ready_;                                       ///< acceptor 状态
    std::function<void(const std::string&)> logger_;            ///< 日志回调
};

// ============================================================================
// Server 构造函数
// ============================================================================

inline Server::Server(unsigned short port)
    : impl_(new Impl(port, "0.0.0.0"))
{
}

inline Server::Server(unsigned short port, const std::string& address)
    : impl_(new Impl(port, address))
{
}

// ============================================================================
// Server 析构函数
// ============================================================================

inline Server::~Server() {
    stop();
}

// ============================================================================
// 注册方法（模板函数实现）
// ============================================================================

template<typename Func>
void Server::register_method(const std::string& name, Func&& func) {
    impl_->get_registry()->register_method(name, std::forward<Func>(func));
}

// ============================================================================
// 运行服务器（阻塞）
// ============================================================================

inline void Server::run() {
    if (!impl_->try_enter_running()) {
        throw std::logic_error("Server is already running");
    }

    struct RunningGuard {
        explicit RunningGuard(Server::Impl* impl)
            : impl_(impl) {}
        ~RunningGuard() {
            if (impl_) {
                impl_->leave_running();
            }
        }
        Server::Impl* impl_;
    } guard(impl_.get());

    impl_->prepare_acceptor();
    // 开始接受连接
    impl_->do_accept();

    // 阻塞运行 I/O 上下文
    impl_->get_io_context().run();
    impl_->get_io_context().restart();
}

// ============================================================================
// 异步启动服务器（非阻塞）
// ============================================================================

inline void Server::start() {
    impl_->start_async();
}

// ============================================================================
// 停止服务器
// ============================================================================

inline void Server::stop() {
    if (impl_) {
        impl_->stop();
    }
}

// ============================================================================
// 获取 io_context
// ============================================================================

inline boost::asio::io_context& Server::get_io_context() {
    return impl_->get_io_context();
}

inline bool Server::is_running() const {
    return impl_->is_running();
}

inline void Server::set_batch_concurrency(std::size_t threads) {
    if (is_running()) {
        throw std::logic_error("服务器正在运行时无法调整批量线程池，请先 stop()");
    }
    impl_->get_registry()->set_batch_concurrency(threads);
}

inline void Server::set_logger(std::function<void(const std::string&)> logger) {
    impl_->set_logger(std::move(logger));
}

} // namespace jsonrpc
