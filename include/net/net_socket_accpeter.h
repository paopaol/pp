#ifndef NET_LISTENER_H
#define NET_LISTENER_H

#include <functional>
#include <hht.h>


namespace pp {
    namespace net {
        typedef std::function<void(int fd)> NewConnHandler;
        typedef std::function<void(const NewConnHandler &handler)>
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