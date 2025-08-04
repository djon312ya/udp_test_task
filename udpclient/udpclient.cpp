#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "udpclient.h"
#include "Vms/Core/Logger.h"

#define _FN "UdpClient"


UdpClient::UdpClient(boost::asio::io_context& io_context, const std::string& host, unsigned short port, double value_to_send)
    : socket_(io_context),
      timer_(io_context),
      value_to_send_(value_to_send),
      total_bytes_to_receive_(TOTAL_DOUBLES * sizeof(double))
{
    // Server endpoint definition
    boost::asio::ip::udp::resolver resolver(io_context);
    server_endpoint_ = *resolver.resolve(boost::asio::ip::udp::v4(), host, std::to_string(port)).begin();

    socket_.open(boost::asio::ip::udp::v4());

    // Start a timer for 3 seconds before sending
    timer_.expires_after(std::chrono::seconds(3));
    timer_.async_wait([this](const boost::system::error_code& error) {
        if (!error) {
            send_request();
        }
    });

    start_receive();
}

void UdpClient::send_request()
{
    auto send_buffer = std::make_shared<std::vector<char>>(sizeof(PacketHeader) + sizeof(double));

    PacketHeader header;
    header.protocol_version = PROTOCOL_VERSION;
    // header.protocol_version = 0; // To test for version errors
    header.message_type = MessageType::REQUEST_DATA;
    header.sequence_number = 0;
    header.payload_size = sizeof(double);

    std::memcpy(send_buffer->data(), &header, sizeof(PacketHeader));
    std::memcpy(send_buffer->data() + sizeof(PacketHeader), &value_to_send_, sizeof(double));

    VMS_LOG_INFO(_FN, "Sending request for data with X = " << value_to_send_);

    socket_.async_send_to(
        boost::asio::buffer(*send_buffer), server_endpoint_,
        [send_buffer](const boost::system::error_code&, size_t){});
}


void UdpClient::start_receive()
{
    socket_.async_receive_from(
        boost::asio::buffer(recv_buffer_), remote_endpoint_,
        [this](const boost::system::error_code& error, size_t bytes_recvd) {
            if (!error && bytes_recvd > 0) {
                handle_receive(bytes_recvd);
            }
            if (received_bytes_count_ < total_bytes_to_receive_) {
                //We keep listening until we get it all
                start_receive();
            }
        });
}

void UdpClient::handle_receive(size_t bytes_recvd)
{
    if (bytes_recvd < sizeof(PacketHeader)) return;

    PacketHeader header;
    std::memcpy(&header, recv_buffer_.data(), sizeof(PacketHeader));

    const char* payload = recv_buffer_.data() + sizeof(PacketHeader);
    const size_t payload_size = bytes_recvd - sizeof(PacketHeader);

    if (header.payload_size != payload_size) return; // Damaged package

    switch (header.message_type)
    {
        case MessageType::DATA_CHUNK:
        {
            if (received_data_.empty()) {
                received_data_.reserve(TOTAL_DOUBLES);
                VMS_LOG_INFO(_FN, "Receiving data from server...");
            }
            // Add the resulting double to our vector
            const size_t doubles_in_chunk = payload_size / sizeof(double);
            received_data_.insert(received_data_.end(),
                                 reinterpret_cast<const double*>(payload),
                                 reinterpret_cast<const double*>(payload) + doubles_in_chunk);

            received_bytes_count_ += payload_size;

            // Check if all data has been received
            if (received_bytes_count_ >= total_bytes_to_receive_)
            {
                process_and_save_data();
            }
            break;
        }
        case MessageType::ERROR_VERSION_MISMATCH:
        {
            VersionMismatchPayload error_payload;
            std::memcpy(&error_payload, payload, sizeof(VersionMismatchPayload));
            VMS_LOG_ERROR(_FN, "Protocol version mismatch!");
            VMS_LOG_ERROR(_FN, "  Client version: " << PROTOCOL_VERSION);
            VMS_LOG_ERROR(_FN, "  Server version: " << error_payload.server_version);
            socket_.close();
            break;
        }
        default:
            VMS_LOG_ERROR(_FN, "  Client version: " << PROTOCOL_VERSION);
            break;
    }
}

void UdpClient::process_and_save_data()
{
    VMS_LOG_INFO(_FN, "All data received. Total doubles: " << received_data_.size() << ".");

    // 1. Sort from largest to smallest
    VMS_LOG_INFO(_FN, "Sorting data...");
    std::sort(received_data_.rbegin(), received_data_.rend());

    // 2. Writing to a binary file
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()) % 1000000;

    std::stringstream ss;
    ss << "log_";
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d_%H-%M-%S");
    ss << "-" << std::setfill('0') << std::setw(6) << us.count();
    ss << ".bin";
    const std::string filename = ss.str();

    VMS_LOG_INFO(_FN, "Saving data to '" << filename << "'...");
    std::ofstream output_file(filename.c_str(), std::ios::binary);
    if (output_file)
    {
        output_file.write(reinterpret_cast<const char*>(received_data_.data()),
                          received_data_.size() * sizeof(double));
        VMS_LOG_INFO(_FN, "Data successfully saved.");
    }
    else
    {
        VMS_LOG_ERROR(_FN, "Error opening output file.");
    }

    socket_.close();
}
