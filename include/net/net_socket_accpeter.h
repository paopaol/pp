#ifndef NET_LISTENER_H
#define NET_LISTENER_H

#include <functional>
#include <hht.h>

namespace pp {
namespace net {
    typedef std::function<void(int fd)> new_conn_handler;
}  // namespace net
}  // namespace pp

#endif
