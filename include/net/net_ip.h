#include "net.h"

#include <vector>

#ifdef WIN32
#include <WinSock2.h>
#else
#endif

namespace pp {
namespace net {
    const auto IPv4len = 4;
    const auto IPv6len = 6;

    class IP {
    public:
        bool        IsGlobalUnicast();
        bool        IsLinkLocalUnicast();
        bool        IsInterfaceLocalMulticast();
        bool        IsLinkLocalMulticast();
        bool        IsMuticast();
        bool        IsLoopback();
        bool        IsUnspecified();
        std::string String();

        bool operator==(const IP& ip2);

    private:
        std::vector<char> ip;
    };

    void ip4_addr(const char* ip, int port, struct sockaddr_in* addr)
    {
        memset(addr, 0, sizeof(*addr));
        addr->sin_family      = AF_INET;
        addr->sin_addr.s_addr = inet_addr(ip);
        addr->sin_port        = htons(port);
    }

}  // namespace net
}  // namespace pp
