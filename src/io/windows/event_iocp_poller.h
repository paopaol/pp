#ifndef EVENT_LISTENER_IOCP_H
#define EVENT_LISTENER_IOCP_H


#include <io/event_loop.h>
#include <errors/hht_error.h>

#ifdef WIN32
#include <windows/event_fd_iocp.h>

#include <WinSock2.h>
#include <Windows.h>
#include <MSWSock.h>
#endif

#include <map>

namespace pp {
    namespace io {



        class IocpEventFd;
        class EventIocpPoller {
        public:
            EventIocpPoller();
            ~EventIocpPoller();
            int Poll(int timeoutms, EventFdList &gotEvents, errors::error_code &error);
            void UpdateEventFd(EventFd *event, errors::error_code &error);
            void RemoveEventFd(EventFd *event, errors::error_code &error);
            void Wakeup();

        private:
           




			EventIocpPoller(const EventIocpPoller&);
			EventIocpPoller &operator=(EventIocpPoller&);


            typedef std::map<int, EventFd *>  EventsMap;


            HANDLE create();
            //int updateEventFd(EventFd *event, errors::error_code &error);


            HANDLE m_iocp;
            //EventVec m_eventsVec;
            EventsMap eventsMap;
        };

    }

}

#endif // EVENT_LISTENER_IOCP_H
