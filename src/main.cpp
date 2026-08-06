#include <iostream>
#include <exception>
#include <boost/asio.hpp>
#include "../include/net/CServer.hpp"
#include "../include/net/AsioIOServicePool.hpp"
#include "../include/net/AsioIOThreadPool.hpp"

int main(int argc, char* argv[]){
    try{
        auto pool = AsioIOServicePool::GetInstance();

        // auto pool = AsioIOThreadPool::GetInstance();
 
        boost::asio::io_context signalContext;
        boost::asio::signal_set signals(signalContext, SIGINT, SIGTERM);
        CServer s(signalContext, 10086);
        
        signals.async_wait([&signalContext, pool](const boost::system::error_code& ec, int signal_number){
            if(ec){
                return;
            }
            // std::cout << "error" << std::endl;
            // pool->stop();没必要在这里stop，因为线程池的析构函数会stop
            signalContext.stop();
            //ec表示错误码，signal_number表示信号编号
        });
        // CServer s(pool->GetIOService(), 10086);
         signalContext.run();
    }
    catch(const std::exception& e){
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    
}

