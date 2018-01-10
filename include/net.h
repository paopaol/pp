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
  }
}

#endif
