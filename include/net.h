#ifndef PP_NET_H
#define PP_NET_H

#include <string>
#include <memory>
#include "errors.h"
#include <_time.h>


namespace pp{
    namespace net{
        class Addr{
        public:
            ~Addr(){};
            virtual std::string NetWork() = 0;
            virtual std::string String() = 0;
        };

        class Conn;
        typedef std::shared_ptr<Conn> ConnRef;
        class Conn{

            friend ConnRef Dial(const std::string &protocol, const std::string &host,_time::Duration timeout, errors::Error &e);
        };


        ConnRef Dial(const std::string &protocol, const std::string &host,_time::Duration timeout, errors::Error &e);
    }
}

#endif