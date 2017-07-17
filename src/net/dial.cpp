#include <net.h>

namespace pp{
    namespace net{
        static auto unkownProtocol = errors::Error("Unkown protocol for net::Dial");

        struct AddrIp4{
            std::string     Ip;
            int             Port;

            std::string String();
        };

        ConnRef Dial(const std::string &protocol, const std::string &host,_time::Duration timeout, errors::Error &e)
        {
            if (protocol == "tcp"){
                return tcpDial(host, timeout, e);
            }else if (protocol == "udp"){
                // return udpDial(host, timeout, e);
            }
            e = unkownProtocol;
            return nullptr;
        }
    }
}