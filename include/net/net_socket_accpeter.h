#ifndef NET_LISTENER_H
#define NET_LISTENER_H

#include <functional>
#include <hht.h>


namespace pp {
    namespace net {
        typedef std::function<void(int fd)> new_conn_handler;
        typedef std::function<void(const new_conn_handler &handler)>
                NewConnHandlerSetter;
        //listen interface
        struct Listener {
			Listener(){}


            NewConnHandlerSetter SetHandleNewConn;
        private:
            DISABLE_COPY_CONSTRCT(Listener);
        };
    }
}

#endif