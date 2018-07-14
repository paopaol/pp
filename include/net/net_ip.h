#include <net.h>

#include <vector>

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
}  // namespace net
}  // namespace pp
