#ifndef NET_LISTENER_H
#define NET_LISTENER_H

#include <functional>
#include <errors/hht_error.h>

namespace pp {
namespace net {
    typedef std::function<void(int fd, const errors::error_code& error)>
        new_conn_handler;
}  // namespace net
}  // namespace pp

#endif
