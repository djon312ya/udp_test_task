/*
 * client.h
 *
 *  Created on: Aug 3, 2025
 *      Author: djon
 */
#ifndef _CLIENT_UDP_CLIENT_H_
#define _CLIENT_UDP_CLIENT_H_

#include <boost/asio.hpp>
#include "common/protocol.h"


class UdpClient
{
    public:
       UdpClient(boost::asio::io_context& io_context, const std::string& host, unsigned short port, double value_to_send);
       ~UdpClient() = default;

    private:
       void send_request();
       void start_receive();
       void handle_receive(size_t bytes_recvd);
       void process_and_save_data();


        boost::asio::ip::udp::socket socket_;
        boost::asio::ip::udp::endpoint server_endpoint_;
        boost::asio::ip::udp::endpoint remote_endpoint_;
        boost::asio::steady_timer timer_;

        double value_to_send_;
        std::array<char, sizeof(PacketHeader) + DATA_CHUNK_PAYLOAD_SIZE> recv_buffer_;
        std::vector<double> received_data_;

        size_t total_bytes_to_receive_;
        size_t received_bytes_count_ = 0;
};

#endif /* _CLIENT_UDP_CLIENT_H_ */
