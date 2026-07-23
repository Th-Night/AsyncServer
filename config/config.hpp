#pragma once

#include <cstddef>

namespace config{
    inline constexpr std::size_t MSG_LEN_FIELD_SIZE = 2;//消息长度的长度
    inline constexpr std::size_t MSG_ID_FIELD_SIZE = 2;//消息id的长度
    inline constexpr std::size_t HEADER_SIZE = MSG_LEN_FIELD_SIZE + MSG_ID_FIELD_SIZE;//包头文件的长度
    inline constexpr std::size_t MAX_SEND_QUE = 1000;//消息队列的长度最大值
    inline constexpr std::size_t MAX_MESSAGE_BODY_SIZE = 1024;//正文消息的最大长度
    inline constexpr std::size_t MAX_PACKET_SIZE = HEADER_SIZE + MAX_MESSAGE_BODY_SIZE;
    inline constexpr std::size_t RECV_BUFFER_SIZE = 4096;//数据区的最大值
}

enum MSG_IDS{
    MSG_HELLO_WORLD = 1001
};