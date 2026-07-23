#include <iostream>
#include <string>
#include <array>
#include <cstring>
#include <boost/asio.hpp>
#include <jsoncpp/json/json.h>
#include "../config/config.hpp"

using boost::asio::ip::tcp;

int main() {
    try {
        boost::asio::io_context io_context;
        tcp::socket socket(io_context);

        socket.connect(tcp::endpoint(boost::asio::ip::make_address("127.0.0.1"), 10086));
        std::cout << "连接服务器成功\n";

        while (true) {
            std::string content;
            std::cout << "请输入聊天内容(q退出): ";
            std::getline(std::cin, content);

            if (content == "q") break;
            if (content.empty()) continue;

            Json::Value root;
            root["from"] = 10086;
            root["to"] = 10087;
            root["content"] = content;

            std::string body = root.toStyledString();

            if (body.size() > config::MAX_MESSAGE_BODY_SIZE) {
                std::cout << "消息太长\n";
                continue;
            }

            uint16_t msg_id = MSG_HELLO_WORLD;
            uint16_t msg_len = static_cast<uint16_t>(body.size());

            uint16_t net_id = boost::asio::detail::socket_ops::host_to_network_short(msg_id);
            uint16_t net_len = boost::asio::detail::socket_ops::host_to_network_short(msg_len);

            std::array<char, config::HEADER_SIZE> header{};
            memcpy(header.data(), &net_id, config::MSG_ID_FIELD_SIZE);
            memcpy(header.data() + config::MSG_ID_FIELD_SIZE, &net_len, config::MSG_LEN_FIELD_SIZE);

            std::array<boost::asio::const_buffer, 2> buffers{
                boost::asio::buffer(header),
                boost::asio::buffer(body)
            };

            boost::asio::write(socket, buffers);

            std::array<char, config::HEADER_SIZE> recv_header{};
            boost::asio::read(socket, boost::asio::buffer(recv_header));

            uint16_t recv_id = 0, recv_len = 0;

            memcpy(&recv_id, recv_header.data(), config::MSG_ID_FIELD_SIZE);
            memcpy(&recv_len, recv_header.data() + config::MSG_ID_FIELD_SIZE, config::MSG_LEN_FIELD_SIZE);

            recv_id = boost::asio::detail::socket_ops::network_to_host_short(recv_id);
            recv_len = boost::asio::detail::socket_ops::network_to_host_short(recv_len);

            std::string recv_body(recv_len, '\0');
            boost::asio::read(socket, boost::asio::buffer(recv_body));

            Json::Value recv_root;
            Json::Reader reader;

            if (reader.parse(recv_body, recv_root)) {
                std::cout << "服务器回复: "
                          << recv_root["content"].asString()
                          << std::endl;
            }
        }
    }
    catch (const std::exception& e) {
        std::cout << "异常:" << e.what() << std::endl;
    }

    return 0;
}