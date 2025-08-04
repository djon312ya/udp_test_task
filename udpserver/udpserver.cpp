#include <algorithm>
#include <iostream>
#include <memory>
#include <unordered_set>
#include <random>
#include <string>
#include <vector>

#include "udpserver.h"
#include "Vms/Core/Logger.h"
#include "common/protocol.h"

#define _FN "UDP_Server"


UdpServer::UdpServer(boost::asio::io_context& io_context, unsigned short port)
: socket_(io_context, boost::asio::ip::udp::endpoint(boost::asio::ip::udp::v4(), port))
{
    VMS_LOG_INFO(_FN, "Server started on port " << port);
    start_receive();
}

void UdpServer::start_receive()
{
    // Waiting to receive data from any client
    socket_.async_receive_from(
        boost::asio::buffer(recv_buffer_), remote_endpoint_,
        [this](const boost::system::error_code& error, size_t bytes_recvd) {
            if (!error && bytes_recvd > 0) {
                handle_request(bytes_recvd);
            }
            start_receive(); // We continue listening
        });
}

void UdpServer::handle_request(size_t bytes_recvd)
{
    if (bytes_recvd < sizeof(PacketHeader))
    {
        return;
    }

    PacketHeader header;
    std::memcpy(&header, recv_buffer_.data(), sizeof(PacketHeader));

    // 1. Checking the protocol version
    if (header.protocol_version != PROTOCOL_VERSION)
    {
        VMS_LOG_ERROR(_FN, "Version mismatch from " << remote_endpoint_ << ". Client uses: "
                      << header.protocol_version << ", server uses: " << PROTOCOL_VERSION);
        send_version_error();
        return;
    }

    // 2. Request processing
    if (header.message_type == MessageType::REQUEST_DATA && header.payload_size == sizeof(double))
    {
        double x;
        std::memcpy(&x, recv_buffer_.data() + sizeof(PacketHeader), sizeof(double));
        VMS_LOG_INFO(_FN, "Received request from " << remote_endpoint_ << " with X = " << x);

        // Create and send data in a separate "session" for each client
        // Use `shared_ptr` for data so it exists while sending
        auto data_to_send = std::make_shared<std::vector<double>>(generate_unique_randoms(x));
        start_sending_data(data_to_send, remote_endpoint_);
    }
}

void UdpServer::send_version_error()
{
    auto send_buffer = std::make_shared<std::vector<char>>(sizeof(PacketHeader) + sizeof(VersionMismatchPayload));

    PacketHeader header;
    header.protocol_version = PROTOCOL_VERSION;
    header.message_type = MessageType::ERROR_VERSION_MISMATCH;
    header.sequence_number = 0;
    header.payload_size = sizeof(VersionMismatchPayload);
    std::memcpy(send_buffer->data(), &header, sizeof(PacketHeader));

    VersionMismatchPayload payload;
    payload.server_version = PROTOCOL_VERSION;
    std::memcpy(send_buffer->data() + sizeof(PacketHeader), &payload, sizeof(VersionMismatchPayload));

    socket_.async_send_to(
        boost::asio::buffer(*send_buffer), remote_endpoint_,
        [send_buffer](const boost::system::error_code&, size_t) {});
}

std::vector<double> UdpServer::generate_unique_randoms(double x)
{
    std::vector<double> values;
    values.reserve(TOTAL_DOUBLES);

    std::mt19937_64 rng(std::chrono::steady_clock::now().time_since_epoch().count());
    std::uniform_real_distribution<double> dist(-x, x);

    // For guaranteed uniqueness, you can use std::unordered_set,
    // but for double the probability of collision is extremely small.
    for (size_t i = 0; i < TOTAL_DOUBLES; ++i)
    {
        values.push_back(dist(rng));
    }
    return values;
}


void UdpServer::start_sending_data(std::shared_ptr<std::vector<double>> data, boost::asio::ip::udp::endpoint target_endpoint)
{
    auto session_data = std::make_shared<SendDataSession>();
    session_data->data_ptr = std::move(data);
    session_data->target_endpoint = target_endpoint;
    session_data->chunk_index = 0;
    send_chunk(session_data);
}


void UdpServer::send_chunk(std::shared_ptr<SendDataSession> session)
{
    const size_t doubles_per_chunk = DATA_CHUNK_PAYLOAD_SIZE / sizeof(double);
    const size_t start_index = session->chunk_index * doubles_per_chunk;

    if (start_index >= session->data_ptr->size())
    {
        VMS_LOG_INFO(_FN, "Finished sending data to " << session->target_endpoint);
        return;
    }

    const size_t doubles_in_this_chunk = std::min(doubles_per_chunk, session->data_ptr->size() - start_index);
    const size_t payload_bytes = doubles_in_this_chunk * sizeof(double);

    auto send_buffer = std::make_shared<std::vector<char>>(sizeof(PacketHeader) + payload_bytes);

    PacketHeader header;
    header.protocol_version = PROTOCOL_VERSION;
    header.message_type = MessageType::DATA_CHUNK;
    header.sequence_number = session->chunk_index;
    header.payload_size = payload_bytes;

    std::memcpy(send_buffer->data(), &header, sizeof(PacketHeader));
    std::memcpy(send_buffer->data() + sizeof(PacketHeader),
                  session->data_ptr->data() + start_index,
                  payload_bytes);

    socket_.async_send_to(
        boost::asio::buffer(*send_buffer), session->target_endpoint,
        [this, session, send_buffer](const boost::system::error_code& error, size_t /*bytes_sent*/) {
            if (!error) {
                session->chunk_index++;
                send_chunk(session); // Send the next chunk
            } else {
                VMS_LOG_ERROR(_FN, "Error sending chunk to " << session->target_endpoint << ": " << error.message());
            }
        });
}

