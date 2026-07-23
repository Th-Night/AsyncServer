#include "../include/MsgNode.hpp"
#include <boost/asio.hpp>
#include "../config/config.hpp"
#include "../include/LogicSystem.hpp"


RecvNode::RecvNode(short max_len, short msg_id):MsgNode(max_len),
_msg_id(msg_id){
}

SendNode::SendNode(const char* msg, short max_len, short msg_id):MsgNode(max_len + config::HEADER_SIZE)
, _msg_id(msg_id){
    //先发送id, 转为网络字节序
    short msg_id_host = boost::asio::detail::socket_ops::host_to_network_short(msg_id);
    memcpy(_data, &msg_id_host, config::MSG_ID_FIELD_SIZE);
    //转为网络字节序
    short max_len_host = boost::asio::detail::socket_ops::host_to_network_short(max_len);
    memcpy(_data + config::MSG_ID_FIELD_SIZE, &max_len_host, config::MSG_LEN_FIELD_SIZE);
    memcpy(_data + config::HEADER_SIZE, msg, max_len);
}