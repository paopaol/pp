#ifndef PP_NET_H
#define PP_NET_H

#include <errors/hht_error.h>
#include <hht.h>



#include <string>
#include <iostream>
#include <sstream>
#include <memory>

namespace pp{
  namespace net{
    struct addr{
        addr(const std::string &ip, int port)
            :Ip(ip)
            ,Port(port)
        {
        }
        addr(){}
        
        std::string string() {
            std::stringstream s;

            s << Ip << ":" << Port;
            return s.str();
        }

        std::string	Ip;
        int         Port;
    };
	
    class socket {
    public:
        explicit socket(int af, int type, int fd);
        ~socket();
        int fd();

        int set_tcp_nodelay(bool set, errors::error_code &error);
        int set_reuse_addr(bool set, errors::error_code &error);
        int set_reuse_port(bool set, errors::error_code &error);
        int set_keep_alive(bool set, errors::error_code &error);
        int set_nonblock(errors::error_code &error);

        addr remote_addr(errors::error_code &error);
        addr local_addr(errors::error_code &error);

        int bind(const addr &addr, errors::error_code &error);
        int listen(errors::error_code &error);

		static int shutdown_write(int fd);
        


        friend int newsocket(int af, int type, errors::error_code &error);
    private:
        static int create(int af, int type, errors::error_code &error);
		

        DISABLE_COPY_CONSTRCT(socket);
        int af_;
        int type_;
        int fd_;
    };

    int newsocket(int af, int type, errors::error_code &error);
  }
}

#endif
