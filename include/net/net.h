#ifndef PP_NET_H
#define PP_NET_H

#include <errors/hht_error.h>
#include <hht.h>

#include <iostream>
#include <memory>
#include <sstream>
#include <string>

namespace pp {
namespace net {
    struct addr {
        addr(const std::string& ip_, int port_) : ip(ip_), port(port_) {}
        addr() {}

        std::string string()
        {
            std::stringstream s;

            s << ip << ":" << port;
            return s.str();
        }

        std::string ip;
        int         port;
    };

    class socket {
    public:
        explicit socket(int af, int type, int fd);
        ~socket();
        int fd();

        int set_tcp_nodelay(bool set, errors::error_code& error);
        int set_reuse_addr(bool set, errors::error_code& error);
        int set_reuse_port(bool set, errors::error_code& error);
        int set_keep_alive(bool set, errors::error_code& error);
        int set_nonblock(errors::error_code& error);

        addr remote_addr(errors::error_code& error);
        addr local_addr(errors::error_code& error);

        int bind(const addr& addr, errors::error_code& error);
        int listen(errors::error_code& error);

        static int shutdown_write(int fd);
		static int close(int fd);

        friend int new_nonblock_socket(int af, int type,
                                       errors::error_code& error);

    private:
        static int create(int af, int type, errors::error_code& error);

        DISABLE_COPY_CONSTRCT(socket);
        int af_;
        int type_;
        int fd_;
    };

    int newsocket(int af, int type, errors::error_code& error);
}  // namespace net
}  // namespace pp

#endif
