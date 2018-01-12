#include <event.h>

#include <event_loop.h>

namespace pp {
namespace io {
Event::Event(EventLoop *loop, int fd)
    :m_eventLoop(loop)
    ,m_fd(fd)
    ,m_trackEvent(EV_NONE)
    ,m_activeEvent(EV_NONE)
{
}

void Event::SetHandleWrite(const DataHandler &handler)
{
    m_handleWrite = handler;
}

void Event::SetHandleRead(const DataHandler &handler)
{
    m_handleRead = handler;
}

void Event::SetHandleClose(const Handler &handler)
{
    m_handleClose = handler;
}

void Event::SetHandleError(const Handler &handler)
{
    m_handleError = handler;
}

void Event::SetActive(int events)
{
    m_activeEvent = events;
}

void Event::TrackRead()
{
    m_trackEvent |= EV_READ;
    trackEvent();
}

void Event::TrackWrite()
{
    m_trackEvent |= EV_WRITE;
    trackEvent();
}
int  Event::GetTracked()
{
    return m_trackEvent;
}

void Event::trackEvent()
{
    m_eventLoop->UpdateEvent(this);
}

EventLoop *Event::ItsLoop()
{
    return m_eventLoop;
}

void Event::HandleEvent(void *data, int len)
{
    if (m_activeEvent & EV_CLOSE &&
            m_handleClose) {
        m_handleClose();
    }
    if (m_activeEvent & EV_ERROR &&
            m_handleError) {
        m_handleError();
    }
    if (m_activeEvent & EV_READ &&
            m_handleRead) {
        m_handleRead(data, len);
    }
    if (m_activeEvent & EV_WRITE &&
            m_handleWrite) {
        m_handleWrite(data, len);
    }
}

}
}
