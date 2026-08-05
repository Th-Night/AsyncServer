#include <iostream>
#include <string>
#include <array>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>
#include <cstring>
#include <boost/asio.hpp>
#include <jsoncpp/json/json.h>
#include "../config/config.hpp"

using boost::asio::ip::tcp;

int main() {
    constexpr int CLIENT_NUM = 100, MSG_NUM = 500;
    std::vector<std::thread> threads;
    std::atomic<int> success{0}, failed{0}, connected{0};
    std::mutex cout_mutex;
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < CLIENT_NUM; ++i) {
        threads.emplace_back([i, &success, &failed, &connected, &cout_mutex]() {
            try {
                boost::asio::io_context io;
                tcp::socket socket(io);
                boost::system::error_code ec;

                socket.connect(tcp::endpoint(boost::asio::ip::make_address("127.0.0.1"), 10086), ec);
                if (ec) {
                    ++failed;
                    std::lock_guard<std::mutex> lock(cout_mutex);
                    std::cerr << "客户端 " << i << " 连接失败：" << ec.message() << '\n';
                    return;
                }

                ++connected;

                for (int j = 0; j < MSG_NUM; ++j) {
                    Json::Value root;
                    root["from"] = i;
                    root["to"] = 0;
                    root["content"] = "hello world";
                    root["sequence"] = j;

                    std::string body = root.toStyledString();
                    uint16_t msg_id = MSG_HELLO_WORLD;
                    uint16_t msg_len = static_cast<uint16_t>(body.size());
                    uint16_t net_id = boost::asio::detail::socket_ops::host_to_network_short(msg_id);
                    uint16_t net_len = boost::asio::detail::socket_ops::host_to_network_short(msg_len);

                    std::array<char, config::HEADER_SIZE> header{};
                    std::memcpy(header.data(), &net_id, config::MSG_ID_FIELD_SIZE);
                    std::memcpy(header.data() + config::MSG_ID_FIELD_SIZE, &net_len, config::MSG_LEN_FIELD_SIZE);

                    std::array<boost::asio::const_buffer, 2> send_buffers{
                        boost::asio::buffer(header),
                        boost::asio::buffer(body)
                    };
                    boost::asio::write(socket, send_buffers);

                    std::array<char, config::HEADER_SIZE> recv_header{};
                    boost::asio::read(socket, boost::asio::buffer(recv_header));

                    uint16_t recv_id = 0, recv_len = 0;
                    std::memcpy(&recv_id, recv_header.data(), config::MSG_ID_FIELD_SIZE);
                    std::memcpy(&recv_len, recv_header.data() + config::MSG_ID_FIELD_SIZE, config::MSG_LEN_FIELD_SIZE);

                    recv_id = boost::asio::detail::socket_ops::network_to_host_short(recv_id);
                    recv_len = boost::asio::detail::socket_ops::network_to_host_short(recv_len);

                    if (recv_len > config::MAX_MESSAGE_BODY_SIZE) {
                        ++failed;
                        break;
                    }

                    std::string recv_body(recv_len, '\0');
                    if (recv_len > 0) boost::asio::read(socket, boost::asio::buffer(recv_body));

                    Json::Value recv_root;
                    Json::Reader reader;

                    if (recv_id == MSG_HELLO_WORLD && reader.parse(recv_body, recv_root)) {
                        ++success;
                    } else {
                        ++failed;
                    }
                }

                socket.shutdown(tcp::socket::shutdown_both, ec);
                socket.close(ec);

                std::lock_guard<std::mutex> lock(cout_mutex);
                std::cout << "客户端 " << i << " 执行结束\n";
            } catch (const std::exception& e) {
                ++failed;
                std::lock_guard<std::mutex> lock(cout_mutex);
                std::cerr << "客户端 " << i << " 异常：" << e.what() << '\n';
            }
        });
    }

    for (auto& thread : threads) {
        if (thread.joinable()) thread.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    std::cout << "\n成功连接:" << connected.load() << '/' << CLIENT_NUM
              << "\n成功消息:" << success.load() << '/' << CLIENT_NUM * MSG_NUM
              << "\n失败数量:" << failed.load()
              << "\n总耗时:" << ms << " ms";

    if (ms > 0) std::cout << "\n吞吐量:" << success.load() * 1000.0 / ms << " 条/秒";
    std::cout << '\n';

    return 0;
}