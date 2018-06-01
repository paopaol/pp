#include <windows/io_win_iocp_poller.h>
#include <io/io_event_fd.h>
#include <io/io_event_poller.h>
#include <windows/io_win_iocp_event_fd.h>
#include <errors/hht_error.h>


#include <assert.h>
#include <memory>


#define NOTFOUND_FROM(map) map.end()
namespace pp {
    namespace io {

	static HANDLE iocpCreate(HANDLE h, HANDLE iocp, DWORD_PTR ptr,
							 errors::error_code &error) {
      auto hiocp = CreateIoCompletionPort(h, iocp, ptr, 0);
      if (hiocp == NULL) {
        int last_error = ::GetLastError();
        error = hht_make_error_code(static_cast<std::errc>(last_error));
        return NULL;
      }
      return hiocp;
        }

        iocp_poller::iocp_poller()
            :m_iocp(NULL)
        {
            m_iocp = create();
            assert(m_iocp != NULL);
            //fixme: add log
        }

        iocp_poller::~iocp_poller()
        {
            assert(m_iocp != NULL);
            ::CloseHandle(m_iocp);
            //fixme: add log
        }

        HANDLE iocp_poller::create()
        {
            return CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);
        }




        void iocp_poller::update_event_fd(event_fd *_event, errors::error_code &error)
        {
            iocp_event_fd *event = static_cast<iocp_event_fd *>(_event);

            //if not found the event fd, we will create it
            auto it = eventsMap.find(event->fd());
            if (it == NOTFOUND_FROM(eventsMap)) {
                iocpCreate((HANDLE)event->fd(), m_iocp, NULL, error);
                if (error.value() == 0) {
                    eventsMap[event->fd()] = event;
                }
                error.clear();
            }


            //now, update it
            if (event->enabled_event() & iocp_event_fd::EV_READ) {
                event->post_read(error);
            }
            if (event->enabled_event() & iocp_event_fd::EV_ACCPET) {
                event->post_accpet(error);
            }

            return;
        }

        void iocp_poller::remove_event_fd(event_fd *event, errors::error_code &error)
        {
			iocp_event_fd *ev = static_cast<iocp_event_fd *>(event);

			auto it = eventsMap.find(ev->fd());
			if (it != NOTFOUND_FROM(eventsMap)){
				eventsMap.erase(it);
			}
        }



        int iocp_poller::poll(int timeoutms, event_fd_list &gotEvents, errors::error_code &error)
        {
            assert(m_iocp);

            DWORD iosize = 0;
            LPWSAOVERLAPPED overlapped = NULL;
            io_request_t *activeRequest = NULL;
            int ecode = 0;
            ULONG_PTR unusedKey = NULL;

            BOOL success = GetQueuedCompletionStatus(m_iocp, &iosize,
                &unusedKey, (LPOVERLAPPED *)&overlapped, timeoutms);
            if (!overlapped && (ecode = ::GetLastError()) != WAIT_TIMEOUT) {
                error = hht_make_error_code(static_cast<std::errc>(ecode));
                return -1;
            }
            activeRequest = CONTAINING_RECORD(overlapped, io_request_t, Overlapped);

            iocp_event_fd  *eventFd = static_cast<iocp_event_fd *>(eventsMap[activeRequest->IoFd]);
            assert(eventFd->fd() == activeRequest->IoFd);
            activeRequest->IoSize = iosize;
            eventFd->set_active_pending(activeRequest);

            eventFd->handle_event();

            return 0;
        }


    }

}
