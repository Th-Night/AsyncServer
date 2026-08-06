#pragma once

#include <iostream>
#include <boost/asio.hpp>
#include <map>
#include <queue>
#include "../common/MsgNode.hpp"
#include "../config/config.hpp"
class CServer;
class CSession :public std::enable_shared_from_this<CSession>
{
    using Strand = boost::asio::strand<boost::asio::io_context::executor_type>;
public:

    CSession(boost::asio::io_context& ioc, CServer* server);
    boost::asio::ip::tcp::socket& Socket();
    ~CSession();
    void Start();
    std::string& GetUuid();
    std::shared_ptr<CSession> SharedSelf();
    void Send(char* msg, short max_length, short msgid);
    void Send(std::string msg, short msgid); 
    void Close();
    std::queue<std::shared_ptr<MsgNode>> _send_que;
    std::mutex _send_lock;
    //收到的消息结构
    std::shared_ptr<RecvNode> _recv_msg_node;
    bool _b_head_parse;
    //收到的头部结构
    std::shared_ptr<MsgNode> _recv_head_node;
    bool _b_close;
    Strand _strand; 

private:
    void HandleRead(const boost::system::error_code& error,
        size_t bytes_transferred, std::shared_ptr<CSession> _self_shared);
    void HandleWrite(const boost::system::error_code& error, std::shared_ptr<CSession> _self_shared);
    
    void AsyncRead();
    std::atomic_bool _closed{false};
    boost::asio::ip::tcp::socket _socket;
    char _data[config::RECV_BUFFER_SIZE];
    CServer* _server;
    std::string _uuid;
};

class LogicNode{
    friend class LogicSystem;
public:
    LogicNode(std::shared_ptr<CSession>, std::shared_ptr<RecvNode>);
private:
    std::shared_ptr<CSession> _session;
    std::shared_ptr<RecvNode> _recvnode;
};