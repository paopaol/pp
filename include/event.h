#ifndef EVENT_H
#define EVENT_H

#include <functional>
#include <vector>

namespace pp {
namespace io {

class EventLoop;

class Event{
public:
    static const int    EV_NONE = 0;
    static const int    EV_READ = 1;
    static const int    EV_WRITE = EV_READ << 2;
    static const int    EV_CLOSE = EV_WRITE << 1;
    static const int    EV_ERROR = EV_CLOSE << 1;

    typedef std::function<void(void *data, int len)> DataHandler;
    typedef std::function<void()> Handler;

    Event(EventLoop *loop, int fd);
    void SetHandleWrite(const DataHandler &handler);
    void SetHandleRead(const DataHandler &handler);
    void SetHandleError(const Handler &handler);
    void SetHandleClose(const Handler &handler);

    void HandleEvent(void *data, int len);
    void SetActive(int events);

    void TrackRead();
    void TrackWrite();
    int  GetTracked();

    inline int Fd() { return m_fd; };

    EventLoop *ItsLoop();

protected:
    void trackEvent();

    EventLoop *m_eventLoop;
    int m_fd;
    int m_trackEvent;
    int m_activeEvent;
    DataHandler m_handleWrite;
    DataHandler m_handleRead;
    Handler m_handleClose;
    Handler m_handleError;
};


}
}

#endif // EVENT_H
