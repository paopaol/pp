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
    struct Addr{
        Addr(const std::string &ip, int port)
            :Ip(ip)
            ,Port(port)
        {
        }
        Addr(){}
        
        std::string string() {
            std::stringstream s;

            s << Ip << ":" << Port;
            return s.str();
        }

        std::string	Ip;
        int         Port;
    };
	
    class Socket {
    public:
        explicit Socket(int af, int type, int fd);
        ~Socket();
        int Fd();

        int SetTcpNodelay(bool set, errors::error_code &error);
        int SetReuseAddr(bool set, errors::error_code &error);
        int SetReusePort(bool set, errors::error_code &error);
        int SetKeepAlive(bool set, errors::error_code &error);
        int SetNonblock(errors::error_code &error);

        Addr RemoteAddr(errors::error_code &error);
        Addr LocalAddr(errors::error_code &error);

        int Bind(const Addr &addr, errors::error_code &error);
        int Listen(errors::error_code &error);

		static int shutdownWrite(int fd);
        


        friend int NewSocket(int af, int type, errors::error_code &error);
    private:
        static int create(int af, int type, errors::error_code &error);
		

        DISABLE_COPY_CONSTRCT(Socket);

        int fd_;
        int af_;
        int type_;
    };

    int NewSocket(int af, int type, errors::error_code &error);
  }
}

#endif
