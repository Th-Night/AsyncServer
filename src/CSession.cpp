#include "../include/CSession.hpp"
#include "../include/CServer.hpp"
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include "../include/LogicSystem.hpp"


CSession::CSession(boost::asio::io_context& ioc, CServer* server) 
: _socket(ioc), _server(server),_b_close(false), _b_head_parse(false), _strand(ioc.get_executor())
{
    boost::uuids::uuid a_uuid = boost::uuids::random_generator()();
    _uuid = boost::uuids::to_string(a_uuid);
    _recv_head_node = std::make_shared<MsgNode>(config::HEADER_SIZE);
}

boost::asio::ip::tcp::socket& CSession::Socket(){
    return _socket;
}

CSession::~CSession(){

}

std::string& CSession::GetUuid(){
    return _uuid;
}

std::shared_ptr<CSession> CSession::SharedSelf(){
    return shared_from_this();
}

void CSession::Send(char* msg, short max_length, short msgid){
    std::lock_guard<std::mutex> lock(_send_lock);
    int send_que_size = _send_que.size();
    if(send_que_size >= config::MAX_SEND_QUE){
        std::cout << "Session: " << _uuid << " send que fulled, size is " << config::MAX_SEND_QUE << std::endl;\
        return;
    }
    _send_que.push(std::make_shared<SendNode>(msg, max_length, msgid));
    if(send_que_size > 0) return;
    auto& msgnode = _send_que.front();
    boost::asio::async_write(
        _socket, 
        boost::asio::buffer(msgnode->_data, msgnode->_total_len),
        // boost::asio::bind_executor(
        //     _strand,
        //     std::bind(
        //         &CSession::HandleWrite,
        //         this,
        //         std::placeholders::_1,
        //         SharedSelf()
        //     ))
        std::bind(
                &CSession::HandleWrite,
                this,
                std::placeholders::_1,
                SharedSelf()
            )
    );  
}

void CSession::HandleWrite(const boost::system::error_code& error, std::shared_ptr<CSession> _self_shared){
    if(error){
        //说明出问题了，会话删
        Close();
        return;
    }
    else{
        std::lock_guard<std::mutex> _lock(_send_lock);
        _send_que.pop();
        if(!_send_que.empty()){
            //当发送队列不为空的时候，就再次调用写事件
            auto& msgnode = _send_que.front();
            boost::asio::async_write(
                _socket, 
                boost::asio::buffer(msgnode->_data, msgnode->_total_len),
                // boost::asio::bind_executor(
                //     _strand,
                //     std::bind(
                //     &CSession::HandleWrite,
                //     this,
                //     std::placeholders::_1,
                //     _self_shared)   
                // )
                std::bind(
                &CSession::HandleWrite,
                this,
                std::placeholders::_1,
                _self_shared
            ));
        }
    }
}

void CSession::AsyncRead(){
    //这里表示连接已经建好可以启动异步读操作
    memset(_data, 0, config::RECV_BUFFER_SIZE);
    _socket.async_read_some(
        boost::asio::buffer(_data, config::RECV_BUFFER_SIZE),
        // boost::asio::bind_executor(
        //     _strand, 
        //     std::bind(
        //         &CSession::HandleRead,
        //         this,
        //         std::placeholders::_1,
        //         std::placeholders::_2,
        //         SharedSelf()
        // ))
        std::bind(
                &CSession::HandleRead,
                this,
                std::placeholders::_1,
                std::placeholders::_2,
                SharedSelf()
            )
    );
}

void CSession::HandleRead(const boost::system::error_code& error,
        size_t bytes_transferred, std::shared_ptr<CSession> _self_shared){
            try{
                if(!error){
                    //已经移动的字符数
                    int copy_len = 0;
                    while(bytes_transferred > 0){
                        if(!_b_head_parse){
                            //收到的数据不足头部大小
                            if(bytes_transferred + _recv_head_node->_cur_len < config::HEADER_SIZE){
                                memcpy(_recv_head_node->_data + _recv_head_node->_cur_len, _data + copy_len, bytes_transferred);
                                _recv_head_node->_cur_len += bytes_transferred;
                                memset(_data, 0, config::RECV_BUFFER_SIZE);
                                _socket.async_read_some(
                                    boost::asio::buffer(_data, config::RECV_BUFFER_SIZE),
                                    // boost::asio::bind_executor(
                                    //     _strand,
                                    //     std::bind(
                                    //         &CSession::HandleRead,
                                    //         this,
                                    //         std::placeholders::_1,
                                    //         std::placeholders::_2,
                                    //         SharedSelf()
                                    //     ))
                                        std::bind(
                                            &CSession::HandleRead,
                                            this,
                                            std::placeholders::_1,
                                            std::placeholders::_2,
                                            SharedSelf()
                                        )
                                    );  
                                    return;
                            }

                            //收到的数据比头节点要多
                            int head_remain = config::HEADER_SIZE - _recv_head_node->_cur_len;
                            memcpy(_recv_head_node->_data + _recv_head_node->_cur_len, _data + copy_len, head_remain);
                            //更新已处理的data长度和剩余未处理的长度
                            copy_len += head_remain;
                            bytes_transferred -= head_remain;

                            //获取头部MSGID数据
                            short msg_id = 0;
                            memcpy(&msg_id, _recv_head_node->_data, config::MSG_ID_FIELD_SIZE);
                            //网络字节序转换为本地字节序
                            msg_id = boost::asio::detail::socket_ops::network_to_host_short(msg_id);
                            //TODO:判断id是否合法，不合法的id就需要删掉此次会话

                            short msg_len = 0;
                            memcpy(&msg_len, _recv_head_node->_data + config::MSG_ID_FIELD_SIZE, config::MSG_LEN_FIELD_SIZE);
                            msg_len = boost::asio::detail::socket_ops::network_to_host_short(msg_len);
                            //TODO:判断长度是否合法

                            _recv_msg_node = std::make_shared<RecvNode>(msg_len, msg_id);
                            //消息的长度小于头部规定的长度，说明数据未授权，则先将部分消息放到接收节点里边
                            if(bytes_transferred < msg_len){
                                memcpy(_recv_msg_node->_data + _recv_msg_node->_cur_len, _data + copy_len, bytes_transferred);
                                _recv_msg_node->_cur_len += bytes_transferred;
                                memset(_data, 0, config::RECV_BUFFER_SIZE);
                                _socket.async_read_some(
                                    boost::asio::buffer(_data, config::RECV_BUFFER_SIZE),
                                    // boost::asio::bind_executor(
                                    //     _strand,
                                    //     std::bind(
                                    //         &CSession::HandleRead,
                                    //         this,
                                    //         std::placeholders::_1,
                                    //         std::placeholders::_2,
                                    //         SharedSelf()
                                    //     )
                                    std::bind(
                                        &CSession::HandleRead,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2,
                                        SharedSelf()
                                    )
                                );
                                _b_head_parse = true;
                                return;
                            }

                            //此时说明消息已经全部收集到了
                            memcpy(_recv_msg_node->_data + _recv_msg_node->_cur_len, _data + copy_len, msg_len);
                            _recv_msg_node->_cur_len += msg_len;
                            copy_len += msg_len;
                            bytes_transferred -= msg_len;
                            _recv_msg_node->_data[_recv_msg_node->_total_len] = '\0';
                            //TODO:处理业务逻辑
                            LogicSystem::GetInstance()->PostMsgToQue(std::make_shared<LogicNode>(SharedSelf(), _recv_msg_node));
                            _b_head_parse = false;
                            _recv_head_node->Clear();
                            if(bytes_transferred <= 0){
                                memset(_data, 0, config::RECV_BUFFER_SIZE);
                                _socket.async_read_some(
                                    boost::asio::buffer(_data, config::RECV_BUFFER_SIZE),
                                    // boost::asio::bind_executor(
                                    //     _strand,
                                    //     std::bind(
                                    //         &CSession::HandleRead,
                                    //         this,
                                    //         std::placeholders::_1,
                                    //         std::placeholders::_2,
                                    //         SharedSelf()
                                    //     )
                                    std::bind(
                                        &CSession::HandleRead,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2,
                                        SharedSelf()
                                    )
                                );
                                return;
                            }
                            continue;
                        }
                        //处理完头部，处理之前未接受的消息数据
                        int remain_msg = _recv_msg_node->_total_len - _recv_msg_node->_cur_len;//待接受的还要这么多
                        //数据传过来不够
                        if(bytes_transferred < remain_msg){
                            memcpy(_recv_msg_node->_data + _recv_msg_node->_cur_len, _data + copy_len, bytes_transferred);
                            _recv_msg_node->_cur_len += bytes_transferred;
                            memset(_data, 0, config::RECV_BUFFER_SIZE);
                            _socket.async_read_some(
                                boost::asio::buffer(_data, config::RECV_BUFFER_SIZE),
                                // boost::asio::bind_executor(
                                //     _strand,
                                //     std::bind(
                                //         &CSession::HandleRead,
                                //         this,
                                //         std::placeholders::_1,
                                //         std::placeholders::_2,
                                //         SharedSelf()
                                //     )
                                std::bind(
                                    &CSession::HandleRead,
                                    this,
                                    std::placeholders::_1,
                                    std::placeholders::_2,
                                    SharedSelf()
                                ));
                            return;
                        }
                        //数据够用了
                        memcpy(_recv_msg_node->_data + _recv_msg_node->_cur_len, _data + copy_len, remain_msg);
                        _recv_head_node->_cur_len += remain_msg;
                        bytes_transferred -= remain_msg;
                        copy_len += remain_msg;
                        _recv_msg_node->_data[_recv_head_node->_total_len] = '\0';
                        
                        //TODO:处理业务逻辑
                        LogicSystem::GetInstance()->PostMsgToQue(std::make_shared<LogicNode>(SharedSelf(), _recv_msg_node));

                        //继续轮询剩余未处理数据
                        _b_head_parse = false;
                        _recv_head_node->Clear();
                        if(bytes_transferred <= 0){
                            memset(_data, 0, config::RECV_BUFFER_SIZE);
                            _socket.async_read_some(
                                boost::asio::buffer(_data, config::RECV_BUFFER_SIZE),
                                // boost::asio::bind_executor(
                                //     _strand,
                                //     std::bind(
                                //     &CSession::HandleRead,
                                //     this,
                                //     std::placeholders::_1,
                                //     std::placeholders::_2,
                                //     SharedSelf()
                                // ))
                                std::bind(
                                    &CSession::HandleRead,
                                    this,
                                    std::placeholders::_1,
                                    std::placeholders::_2,
                                    SharedSelf()
                                )
                            );
                            return;
                        }
                        continue;

                    }
                }
                else{
                    //TODO:关闭会话
                    Close();
                    //TOAD:删除uuid
                }
            }
            catch(std::exception& e){
                std::cout << "Exception code is " << e.what() <<std::endl;
            }
        }

void CSession::Close(){
    // 防止读回调、写回调同时重复关闭
    bool expected = false;
    if (!_closed.compare_exchange_strong(expected, true)) {
        return;
    }

    boost::system::error_code ec;

    // 取消尚未完成的异步操作
    _socket.cancel(ec);

    ec.clear();

    // 关闭TCP双向收发
    _socket.shutdown(
        boost::asio::ip::tcp::socket::shutdown_both,
        ec
    );

    ec.clear();

    // 关闭socket句柄
    _socket.close(ec);

    // 从服务器的会话表中删除
    if (_server != nullptr) {
        _server->ClearSession(_uuid);
    }
}

void CSession::Start(){
    AsyncRead();
}

void CSession::Send(std::string msg, short msgid) {
    std::lock_guard<std::mutex> lock(_send_lock);
    int send_que_size = _send_que.size();
    if (send_que_size > config::MAX_SEND_QUE) {
        std::cout << "session: " << _uuid << " send que fulled, size is " << config::MAX_SEND_QUE << std::endl;
        return;
    }
    _send_que.push(std::make_shared<SendNode>(msg.c_str(), msg.length(), msgid));
    if (send_que_size > 0) {
        return;
    }
    auto& msgnode = _send_que.front();
    boost::asio::async_write(
        _socket, 
        boost::asio::buffer(msgnode->_data, msgnode->_total_len),
        // boost::asio::bind_executor(_strand, std::bind(&CSession::HandleWrite, this, std::placeholders::_1, SharedSelf())));
        std::bind(&CSession::HandleWrite, this, std::placeholders::_1, SharedSelf()));
        
}

LogicNode::LogicNode(std::shared_ptr<CSession> session, std::shared_ptr<RecvNode> recvnode) : _session(session), _recvnode(recvnode){
    
}