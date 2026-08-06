#pragma once
#include <boost/asio.hpp>
#include <atomic>

#include "../common/Singleton.hpp"

class AsioIOThreadPool : public Singleton<AsioIOThreadPool>{
public:
    using Work = boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;
    friend class Singleton<AsioIOThreadPool>;
    ~AsioIOThreadPool();
    AsioIOThreadPool& operator=(const AsioIOThreadPool&) = delete;
    AsioIOThreadPool(const AsioIOThreadPool&) = delete;
    boost::asio::io_context& GetIOService();
    void stop();
private:
    AsioIOThreadPool(size_t threadNum = std::thread::hardware_concurrency());
    boost::asio::io_context _service;//注意此处必须他在前
    std::atomic_bool _stopped{false};
    std::unique_ptr<Work> _work;
    std::vector<std::thread> _threads;
};