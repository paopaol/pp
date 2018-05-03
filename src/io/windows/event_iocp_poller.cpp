#include <windows/event_iocp_poller.h>
#include <io/event_fd.h>
#include <io/event_poller.h>
#include <errors/hht_error.h>


#include <assert.h>
#include <memory>


#define NOTFOUND_FROM(map) map.end()
namespace pp {
    namespace io {

        static HANDLE  iocpCreate(HANDLE h, HANDLE iocp, DWORD_PTR ptr, errors::error_code &error)
        {
            auto hiocp = CreateIoCompletionPort(h, iocp, ptr, 0);
            if (hiocp == NULL) {
                int last_error = ::GetLastError();
				error = hht_make_error_code(static_cast<std::errc>(last_error));
                return NULL;
            }
            return hiocp;
        }

        EventIocpPoller::EventIocpPoller()
            :m_iocp(NULL)
        {
            m_iocp = create();
            assert(m_iocp != NULL);
            //fixme: add log
        }

        EventIocpPoller::~EventIocpPoller()
        {
            assert(m_iocp != NULL);
            CloseHandle(m_iocp);
            //fixme: add log
        }

        HANDLE EventIocpPoller::create()
        {
            return CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);
        }




        void EventIocpPoller::UpdateEventFd(EventFd *_event, errors::error_code &error)
        {
            IocpEventFd *event = static_cast<IocpEventFd *>(_event);

            auto it = eventsMap.find(event->Fd());
            if (it == NOTFOUND_FROM(eventsMap)) {
                iocpCreate((HANDLE)event->Fd(), m_iocp, NULL, error);
                if (error.value() == 0) {
                    eventsMap[event->Fd()] = event;
                }
                error.clear();
            }
  
                    

            if (event->GetEnabledEvent() & IocpEventFd::EV_READ /*&& !event->HasBeenEnableRead()*/) {
                event->PostRead(error);
            }
            if (event->GetEnabledEvent() & IocpEventFd::EV_ACCPET) {
                event->PostAccpet(error);
            }

            return;
        }

        void EventIocpPoller::RemoveEventFd(EventFd *event, errors::error_code &error)
        {
			IocpEventFd *ev = static_cast<IocpEventFd *>(event);

			auto it = eventsMap.find(ev->Fd());
			if (it != NOTFOUND_FROM(eventsMap)){
				eventsMap.erase(it);
			}
        }

 

        int EventIocpPoller::Poll(int timeoutms, EventFdList &gotEvents, errors::error_code &error)
        {
            assert(m_iocp);

            DWORD iosize = 0;
            LPWSAOVERLAPPED overlapped = NULL;
            IoRequest *activeRequest = NULL;
            int ecode = 0;
            ULONG_PTR unusedKey = NULL;

            BOOL success = GetQueuedCompletionStatus(m_iocp, &iosize,
                &unusedKey, (LPOVERLAPPED *)&overlapped, timeoutms);
            if (!overlapped && (ecode = ::GetLastError()) != WAIT_TIMEOUT) {
                error = hht_make_error_code(static_cast<std::errc>(ecode));
                return -1;
            }
            activeRequest = CONTAINING_RECORD(overlapped, IoRequest, Overlapped);

            IocpEventFd  *eventFd = static_cast<IocpEventFd *>(eventsMap[activeRequest->IoFd]);
            assert(eventFd->Fd() == activeRequest->IoFd);
            activeRequest->IoSize = iosize;
            eventFd->setActivePending(activeRequest);

            eventFd->HandleEvent();


#if 0
            if (pendingRequest->IoOpt & IocpEventFd::EV_WAKEUP) {
                //handleWakeUp();
                return 0;
            }else if (pendingRequest->IoOpt & IocpEventFd::EV_READ) {
                handleReadIoOpt(eventFd, ioCtxptr, iosize);
            }
            else if (pendingRequest->IoOpt & IocpEventFd::EV_ACCPET) {
                handleAcceptIoOpt(eventFd, fdCtx_, ioCtxptr);
            }
            else if (pendingRequest->IoOpt & IocpEventFd::EV_WRITE) {
                handleWritetIoOpt(eventFd, ioCtxptr, iosize);
            }
#endif
            return 0;
        }


    }

}
