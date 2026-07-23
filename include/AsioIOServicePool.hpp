#pragma once
#include "Singleton.hpp"
#include <boost/asio.hpp>
#include <vector>

class AsioIOServicePool : public Singleton<AsioIOServicePool>{
    friend Singleton<AsioIOServicePool>;
public:
    using IOService = boost::asio::io_context;
    using Work = boost::asio::executor_work_guard<boost::asio::io_context::executor_type>;
    using WorkPtr = std::unique_ptr<Work>;
    ~AsioIOServicePool();
    AsioIOServicePool(const AsioIOServicePool&) = delete;
    AsioIOServicePool& operator= (const AsioIOServicePool&) = delete;
    //使用round-robin的方式返回一个io_context
    boost::asio::io_context& GetIOService();
    void stop();
private:
    AsioIOServicePool(std::size_t size = std::thread::hardware_concurrency());//后边的函数就是返回并行数，其实就是cpu的核数
    std::vector<IOService> _ioService;
    std::vector<WorkPtr> _works;
    std::vector<std::thread> _threads;
    std::size_t _nextIOService;//记一下上一次返回的io_context的坐标
};