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
				error = hht_make_error_code(static_cast<std::errc>(::WSAGetLastError()));
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
            IocpEventFd::fdCtxRef ctx;

            if (event->GetEnabledEvent() & EventFd::EV_READ && !event->HasBeenEnableRead()) {
                event->PreparefdCtx(EventFd::EV_READ);
                ctx = event->FdCtxRef();
            }
            if (event->GetEnabledEvent() & IocpEventFd::EV_WAKEUP) {
                event->PreparefdCtx(IocpEventFd::EV_WAKEUP);
                ctx = event->FdCtxRef();
            }
            if (event->GetEnabledEvent() & IocpEventFd::EV_ACCPET) {
                event->PreparefdCtx(IocpEventFd::EV_ACCPET);
                ctx = event->FdCtxRef();
            }

            if (ctx) {
                auto ev = eventsMap.find(event->Fd());
                if (ev == NOTFOUND_FROM(eventsMap)) {
                    m_iocp = iocpCreate((HANDLE)event->Fd(), m_iocp, (DWORD_PTR)ctx.get(), error);
                    hht_return_none_if_error(error);
                    eventsMap[event->Fd()] = event;
                }

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
            fdCtx *fdCtx_ = NULL;
            DWORD recvNumBytes = 0;
            DWORD sendNumBytes = 0;
            LPWSAOVERLAPPED overlapped = NULL;
            DWORD flags = 0;
            ioCtx *ioCtxptr = NULL;

            BOOL success = GetQueuedCompletionStatus(m_iocp, &iosize,
                (PDWORD_PTR)&fdCtx_, (LPOVERLAPPED *)&overlapped, timeoutms);
            if (!success && !overlapped) {
                error = hht_make_error_code(static_cast<std::errc>(::WSAGetLastError()));
                return -1;
            }

            ioCtxptr = CONTAINING_RECORD(overlapped, ioCtx, Overlapped);

            if (!fdCtx_) {
                error = hht_make_error_code(static_cast<std::errc>(GetLastError()));
                return -1;
            }

            IocpEventFd  *eventFd = static_cast<IocpEventFd *>(eventsMap[fdCtx_->Fd]);
            assert(eventFd->Fd() == fdCtx_->Fd);
            if (fdCtx_->ReadIoCtx.IoOpt & IocpEventFd::EV_WAKEUP) {
                return 0;
            }
            if (iosize == 0 && !(fdCtx_->ReadIoCtx.IoOpt & IocpEventFd::EV_ACCPET)) {
                eventFd->SetActive(IocpEventFd::EV_CLOSE);
                eventFd->HandleEvent(NULL, 0);
                return 0;
            }
            int ret = 0;

            if (!ioCtxptr) {
                return 0;
            }


            if (ioCtxptr->IoOpt & IocpEventFd::EV_READ) {
                eventFd->SetActive(IocpEventFd::EV_READ);
                eventFd->HandleEvent(ioCtxptr->Wsabuf.buf, iosize);
            }
            else if (ioCtxptr->IoOpt & IocpEventFd::EV_ACCPET) {
                eventFd->SetActive(IocpEventFd::EV_ACCPET);
                int acpepetFd = ioCtxptr->FdAccpet;
                int fd = fdCtx_->Fd;
                eventFd->EnableAccpet(error);
                //closesocket(acpepetFd);
               eventFd->HandleAccpetEvent(acpepetFd, fdCtx_->Fd);
            }
            else if (ioCtxptr->IoOpt & IocpEventFd::EV_WRITE) {
                eventFd->SetActive(IocpEventFd::EV_WRITE);
                //
                // a write operation has completed, determine if all the data intended to be
                // sent actually was sent.
                //
                ioCtxptr->IoOpt = IocpEventFd::EV_WRITE;
                ioCtxptr->SentBytes += iosize;
                flags = 0;
                if (ioCtxptr->SentBytes < ioCtxptr->TotalBytes) {
                    //
                    // the previous write operation didn't send all the data,
                    // post another send to complete the operation
                    //
                    eventFd->PostWrite(ioCtxptr->Buffer + ioCtxptr->SentBytes,
                        ioCtxptr->TotalBytes - ioCtxptr->SentBytes, error);
                    //bufsend.buf = ioCtxptr->Buffer + ioCtxptr->SentBytes;
                    //bufsend.len = ioCtxptr->TotalBytes - ioCtxptr->SentBytes;
                    //ret = WSASend(fdCtx_->Fd, &bufsend, 1,
                    //    &sendNumBytes, flags, &(ioCtxptr->Overlapped), NULL);
                    //if (ret == SOCKET_ERROR &&
                    //    (ERROR_IO_PENDING != WSAGetLastError())) {
                    //    eventFd->SetActive(IocpEventFd::EV_ERROR);
                    //    eventFd->HandleEvent(NULL, 0);
                    //}
                }
                else {
                    eventFd->SetActive(IocpEventFd::EV_WRITE);
                    eventFd->HandleEvent(NULL, 0);
                }
            }

            return 0;
        }


    }

}
