#ifndef EVENT_LISTENER_IOCP_H
#define EVENT_LISTENER_IOCP_H


#include <event_loop.h>
#include "event_iocp.h"

#include <WinSock2.h>
#include <Windows.h>
#include <MSWSock.h>

#include <map>

namespace pp {
namespace io {
class IocpListener{
public:
    IocpListener();
    int Listen(int timeoutms, EventVec &gotEvents);
    void AddEvent(Event *event);
    void RemoveEvent(Event *event);

private:
    #define MAX_BUFF_SIZE  8192

    struct ioCtx {
        WSAOVERLAPPED               Overlapped;
        char                        Buffer[MAX_BUFF_SIZE];
        WSABUF                      Wsabuf;
        int                         TotalBytes;
        int                         SentBytes;
        int 			            IoOpt;
        int                         FdAccpet;
    };
    typedef std::map<int, Event *>  EventsMap;


    struct fdCtx {
        int     Fd;
        ioCtx  	IoCtx;
    };
    HANDLE create();
    int preparefdCtx(fdCtx *fdCtx_, int fd, int ev);
    int initAccpet(fdCtx *fdCtx_);


    HANDLE m_iocp;
    //EventVec m_eventsVec;
    EventsMap m_eventsMap;
};

}

}

#endif // EVENT_LISTENER_IOCP_H
