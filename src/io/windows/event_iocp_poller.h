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
#define MAX_WSA_BUFF_SIZE  (2 * (sizeof(SOCKADDR_STORAGE) + 16) + 5)
        struct ioCtx {
			ioCtx(){}

            WSAOVERLAPPED               Overlapped;
            char                        Buffer[MAX_WSA_BUFF_SIZE];
            WSABUF                      Wsabuf;
            int                         TotalBytes;
            int                         SentBytes;
            int 			            IoOpt;
            int                         FdAccpet;

		private:
			ioCtx(const ioCtx&);
			ioCtx &operator=(const ioCtx&);
        };

        struct fdCtx {
			fdCtx(){}
            ~fdCtx() {}

            int     Fd;
            ioCtx  	ReadIoCtx;
            ioCtx   WriteIoCtx;

		private:
			fdCtx(const fdCtx&);
			fdCtx &operator=(const fdCtx&);
        };

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
