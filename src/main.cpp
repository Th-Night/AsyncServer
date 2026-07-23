#include <iostream>
#include <exception>
#include <boost/asio.hpp>
#include "../include/CServer.hpp"


int main(int argc, char* argv[]){
    try{
        boost::asio::io_context  io_context;
        boost::asio::signal_set signals(io_context, SIGINT, SIGTERM);
        signals.async_wait([&io_context](const boost::system::error_code& ec, int signal_number){
            io_context.stop();
        });//ec表示错误码，signal_number表示信号编号
        CServer s(io_context, 10086);
        io_context.run();
    }
    catch(const std::exception& e){
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    
}

