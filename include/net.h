#ifndef PP_NET_H
#define PP_NET_H

#include <string>
#include <memory>
#include "errors.h"
#include <_time.h>


namespace pp{
  namespace net{
    struct Addr{
      std::string	Ip;
      int         Port;
    };
	
    class Socket {
    public:
        int Fd();

        int SetSocketOpt();
        


        friend Socket NewSocket();
        friend Socket NewNonblockSocket();
    private:
        int create(errors::Error &e);

        int m_fd;
    };

	Socket NewSocket();
    Socket NewNonblockSocket();
  }
}

#endif
