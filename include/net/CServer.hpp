#pragma once

#include <boost/asio.hpp>
#include <memory>
#include <map>
#include <atomic>
#include <mutex>
#include <string>
class CSession;
class CServer
{
public:
    CServer(boost::asio::io_context& io_context, short port);
    void ClearSession(const std::string& uuid);
private:
    void HandleAccept(std::shared_ptr<CSession> new_ession, const boost::system::error_code & error);
    void StartAccept();
    boost::asio::io_context &_ioc;
    short _port;
    boost::asio::ip::tcp::acceptor _acceptor;
    std::map<std::string, std::shared_ptr<CSession>> _sessions;
    std::mutex _sessions_lock;
};