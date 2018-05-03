#ifndef EVENT_IOCP_H
#define EVENT_IOCP_H

#include <io/event_fd.h>
#include <errors/hht_error.h>


#include <functional>
#include <memory>
#include <map>

#ifdef WIN32
#include <windows/event_iocp_poller.h>
#include <WinSock2.h>
#include <Windows.h>
#include <MSWSock.h>

#endif
namespace pp {
    namespace io {

        typedef std::function<void(int)> AccpetEventHandler;

        struct IoRequest;
        class IocpEventFd :public EventFd {
        public:
            typedef std::shared_ptr<struct IoRequest> IoRequestRef;

            static const int EV_ACCPET = EventFd::EV_ERROR << 1; //only for windows
            static const int EV_WAKEUP = IocpEventFd::EV_ACCPET << 1; //only for windows

            IocpEventFd(EventLoop *loop, int fd);

            ~IocpEventFd() { }
            void EnableAccpet(errors::error_code &error);
            void DisableRead();
            void EnableWakeUp(errors::error_code &error);
            void SetAccpetEventHandler(const AccpetEventHandler &handler);


            void HandleEvent();
            void HandleAccpetEvent(int fd);
            int handleReadDone();
            int handleAcceptDone();
            int handleWriteDone();
            int handleZero();

            int PostRead(errors::error_code &error);
            int PostWrite(const void *data, int len, errors::error_code &error);
            int PostAccpet(errors::error_code &error);


            IoRequestRef removeActiveRequest()
            {
                IoRequestRef active;
                active.swap(activePendingReq);
                return active;
            }
            int PendingRequestSize()
            {
                return  ioRequestList.size();
            }

            void setActivePending(IoRequest *active)
            {
                auto find = ioRequestList.find(active);
                assert(find != ioRequestList.end());
                activePendingReq = find->second;
                ioRequestList.erase(find);
            }


            IoRequestRef createIoRequest(int ev);
            void queuedPendingRequest(const IoRequestRef request) {
                ioRequestList[request.get()] = request;
            }



        private:
			IocpEventFd(const IocpEventFd&);
			IocpEventFd &operator=(const IocpEventFd&);



            AccpetEventHandler                          m_handleAccpetEvent;
            std::map<IoRequest *, IoRequestRef>         ioRequestList;

            IoRequestRef                                activePendingReq;
        };


#define MAX_WSA_BUFF_SIZE  (2 * (sizeof(SOCKADDR_STORAGE) + 16) + 5)
        struct IoRequest {
            IoRequest()
                :TotalBytes(0)
                , SentBytes(0)
                , IoOpt(IocpEventFd::EV_NONE)
                , AccpetFd(-1)
                , IoFd(-1)
                , IoSize(0)
            {
                ZeroMemory(&Overlapped, sizeof(Overlapped));
                Wsabuf.buf = Buffer;
                Wsabuf.len = sizeof(Buffer);
                ZeroMemory(Wsabuf.buf, Wsabuf.len);
            }

            WSAOVERLAPPED               Overlapped;
            char                        Buffer[MAX_WSA_BUFF_SIZE];
            WSABUF                      Wsabuf;
            int                         TotalBytes;
            int                         SentBytes;
            int 			            IoOpt;
            int                         AccpetFd;
            int                         IoFd;
            int                         IoSize;  //GetQueuedCompletionStatus  second arg

        private:
            IoRequest(const IoRequest&);
            IoRequest &operator=(const IoRequest&);
        };
    }
}

#endif