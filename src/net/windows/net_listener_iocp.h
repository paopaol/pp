#ifndef NET_LISTENER_IOCP_H
#define NET_LISTENER_IOCP_H
#include <net_listener.h>


#include <event_iocp.h>

namespace pp {
    namespace net {
        class io::EventLoop;
        class IocpListener {
            IocpListener(io::EventLoop *loop);

            void SetHandleNewConn(const NewConnHandler &handler);

        private:
            void handleAccpetEvent();

        private:
            io::IocpEvent m_event;
            io::EventLoop *m_loop;
            NewConnHandler m_handleNewConn;
        };
    }
}

#endif