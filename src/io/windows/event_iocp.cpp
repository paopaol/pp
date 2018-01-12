#include "event_iocp.h"

#include <event_loop.h>

namespace pp {
    namespace io {
        IocpEvent::IocpEvent(EventLoop *loop, int fd)
            :Event(loop, fd)
        {
        }


        void IocpEvent::TrackAccpet()
        {
            m_trackEvent |= IocpEvent::EV_ACCPET;
            m_eventLoop->UpdateEvent(this);
        }

        void IocpEvent::HandleEvent(void *data, int len)
        {
            Event::HandleEvent(data, len);
        }
    }
}