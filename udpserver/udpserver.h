/*
 * udpserver.h
 *
 *  Created on: Aug 3, 2025
 *      Author: djon
 */
#ifndef _SERVER_UDP_SERVER_H_
#define _SERVER_UDP_SERVER_H_

#include <boost/asio.hpp>

constexpr uint16_t UDP_PKT_MAX_SIZE = 65535;

class UdpServer
{
    public:
        UdpServer(boost::asio::io_context& io_context, unsigned short port);
        ~UdpServer() = default;

    private:
        struct SendDataSession {
            std::shared_ptr<std::vector<double>> data_ptr;
            boost::asio::ip::udp::endpoint target_endpoint;
            uint32_t chunk_index = 0;
        };

        void start_receive();
        void handle_request(size_t bytes_recvd);
        void send_version_error();
        std::vector<double> generate_unique_randoms(double x);
        void start_sending_data(std::shared_ptr<std::vector<double>> data, boost::asio::ip::udp::endpoint target_endpoint);
        void send_chunk(std::shared_ptr<SendDataSession> session);

        boost::asio::ip::udp::socket socket_;
        boost::asio::ip::udp::endpoint remote_endpoint_;
        std::array<char, UDP_PKT_MAX_SIZE> recv_buffer_;
};

#endif /* _CLIENT_UDP_CLIENT_H_ */
