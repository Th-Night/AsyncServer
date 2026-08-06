#include "../../include/net/AsioIOServicePool.hpp"

AsioIOServicePool::AsioIOServicePool(std::size_t size) : 
_ioServices(size), _works(size), _nextIOService(0){

    for(std::size_t i = 0; i < size; i++){
        _works[i] = std::unique_ptr<Work>(new Work(_ioServices[i].get_executor()));
    }

    //遍历多个io_service,创建多个线程，每个线程内部启动ioservice
    for(std::size_t i = 0; i < _ioServices.size(); i++){
        _threads.emplace_back([this, i](){
            _ioServices[i].run();
        });
    }
}

AsioIOServicePool::~AsioIOServicePool(){
    std::cout << "AsioIOServicePool destruct" << std::endl;
    stop();//其实这里建立stop()的话主线程就不应该在stop(),因为当线程池析构的时候这里又会进行一次析构
}

boost::asio::io_context& AsioIOServicePool::GetIOService(){
    auto& service = _ioServices[_nextIOService++];
    _nextIOService %= _ioServices.size();
    return service;
}

void AsioIOServicePool::stop(){
    for(auto& work : _works){
        work.reset();
    }
    for(auto& t : _threads) {
        if(t.joinable()){
            t.join();
        }

    }
}
