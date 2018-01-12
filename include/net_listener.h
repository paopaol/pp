#ifndef NET_LISTENER_H
#define NET_LISTENER_H

#include <functional>


namespace pp {
    namespace net {
        typedef std::function<void(int fd)> NewConnHandler;
        typedef std::function<void(const NewConnHandler &handler)>
                NewConnHandlerSetter;
        //listen interface
        struct Listener {
            NewConnHandlerSetter SetHandleNewConn;
        };
    }
}

#endif