#include "../include/AsioIOThreadPool.hpp"

AsioIOThreadPool::AsioIOThreadPool(size_t threadNum) 
:_work(std::make_unique<Work>(boost::asio::make_work_guard(_service))){
    for(size_t i = 0; i < threadNum; ++i){
        _threads.emplace_back([this](){
            _service.run();
        });
    }
}

boost::asio::io_context& AsioIOThreadPool::GetIOService(){
    return _service;
}

void AsioIOThreadPool::stop(){
    if(_stopped.exchange(true)) return;//防止重复stop()
    _work.reset();//执行这个之后就会等io_context没有事件之后就返回
    _service.stop();
    //这个reset是unique_ptr的方法，可以释放掉这个指针
    for(auto& t : _threads){
        if(t.joinable()){
            t.join();
        }       
    }
} 