#include "net_listener_iocp.h"

#include <net_listener.h>
#include <event.h>
#include <event_loop.h>
#include <net.h>

#include <_time.h>

namespace pp {
    namespace net {
        IocpListener::IocpListener(io::EventLoop *loop)
            :m_loop(loop)
            ,m_event(loop, NewSocket().Fd())
        {
            m_event.TrackRead();
        }

        void IocpListener::SetHandleNewConn(const NewConnHandler &handler) {
            m_handleNewConn = handler;
        }

        void IocpListener::handleAccpetEvent() {
            int fd;
            m_handleNewConn(fd);
        }
    }
}