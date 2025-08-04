#include "udpclient.h"
#include "Vms/Core/Logger.h"
#include "common/config_parser.h"

#define _FN "Client"

int main(int argc, char* argv[])
{
    Vms::Core::logger.setLevel(Vms::Core::LogLevelDEBUG);
    Vms::Core::logger.setVerbose(true);
    try
    {
        auto config = parse_config("client.conf");
        std::string host = config.at("host");
        unsigned short port = std::stoi(config.at("port"));
        double value_x = std::stod(config.at("value_x"));

        boost::asio::io_context io_context;
        UdpClient client(io_context, host, port, value_x);
        io_context.run();
    }
    catch (const std::exception& e)
    {
        VMS_LOG_ERROR(_FN, "Exception: " << e.what());
    }
    return 0;
}
