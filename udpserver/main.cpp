#include "udpserver.h"
#include "Vms/Core/Logger.h"
#include "common/config_parser.h"

#define _FN "UDP_Server"

int main(int argc, char* argv[])
{
    Vms::Core::logger.setLevel(Vms::Core::LogLevelDEBUG);
    Vms::Core::logger.setVerbose(true);
    try
    {
        auto config = parse_config("server.conf");
        unsigned short port = std::stoi(config.at("port"));

        boost::asio::io_context io_context;
        UdpServer server(io_context, port);
        io_context.run();
    }
    catch (const std::exception& e)
    {
        VMS_LOG_ERROR(_FN, "Exception: " << e.what());
    }
    return 0;
}
