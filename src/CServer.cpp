#include "../include/CServer.hpp"
#include "../include/CSession.hpp"
CServer::CServer(boost::asio::io_context& io_context, short port) : _ioc(io_context), _port(port),
_acceptor(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)){
    StartAccept();
}

void CServer::ClearSession(const std::string& uuid) {
    std::lock_guard<std::mutex> lock(_sessions_lock);

    auto iter = _sessions.find(uuid);
    if (iter != _sessions.end()) {
        _sessions.erase(iter);
    }
}

void CServer::StartAccept(){
    //这里要开始建立连接，就是异步监听
    std::shared_ptr<CSession> new_session = std::make_shared<CSession>(_ioc, this); 
    _acceptor.async_accept(
        new_session->SharedSelf()->Socket(),
        std::bind(
            &CServer::HandleAccept,
            this,
            new_session,
            std::placeholders::_1
        )
    );
}

void CServer::HandleAccept(std::shared_ptr<CSession> new_session, const boost::system::error_code& ec){
    if(!ec){
        new_session->Start();
        _sessions.insert(make_pair(new_session->GetUuid(), new_session));
    }
    StartAccept();//处理完成之后就要开始新的监听
}