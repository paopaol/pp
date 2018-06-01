
#ifdef WIN32
#include "windows/io_win_iocp_event_fd.h"
#endif

#include <io/io_event_loop.h>

#include <memory>
#include <system/sys_mutex.h>
#include <net/net.h>

using namespace std;


#define SUCCEEDED_WITH_IOCP(result)                                  \
  ((result) || ((::WSAGetLastError()) == ERROR_IO_PENDING))

namespace pp {
    namespace io {

        iocp_event_fd::iocp_event_fd(event_loop *loop, int fd)
            :event_fd(loop, fd)
        {
        }

        void iocp_event_fd::enable_accpet(errors::error_code &error)
        {
            enabled_event_ |= iocp_event_fd::EV_ACCPET;
            event_loop_->update_event_fd(this, error);
        }

        void iocp_event_fd::enable_wakeup(errors::error_code &error)
        {
            enabled_event_ |= iocp_event_fd::EV_WAKEUP;
            event_loop_->update_event_fd(this, error);
        }

        void iocp_event_fd::set_accpet_event_handler(const AccpetEventHandler &handler)
        {
            m_handleAccpetEvent = handler;
        }

        void iocp_event_fd::handle_accpet_event(int fd)
        {
            if (m_handleAccpetEvent) {
                m_handleAccpetEvent(fd);
            }
        }

        int iocp_event_fd::post_read(errors::error_code &error)
        {
            DWORD recvBytes = 0;
            DWORD flags = 0;

            io_request_ref request = create_io_request(event_fd::EV_READ);
            int ret = WSARecv(fd_, &request.get()->Wsabuf,
                1, &recvBytes, &flags, &request.get()->Overlapped, NULL);
            if (!SUCCEEDED_WITH_IOCP(ret == 0)) {
                int code = ::GetLastError();
                error = hht_make_error_code(static_cast<std::errc>(code));
                return -1;
            }
            queued_pending_request(request);
            return 0;
        }


        int iocp_event_fd::handle_zero()
        {
            set_active(iocp_event_fd::EV_CLOSE);
            event_fd::handle_event();
            return 0;
        }

        int iocp_event_fd::handle_read_done()
        {
            assert(activePendingReq != nullptr);

            if (activePendingReq->IoSize == 0) {
                handle_zero();
                return 0;
            }
            set_active(iocp_event_fd::EV_READ);
            event_fd::handle_event();
            return 0;
        }



        int iocp_event_fd::handle_accept_done()
        {
            errors::error_code error;

            int acpepetFd = activePendingReq->AccpetFd;
            int listenFd = activePendingReq->IoFd;

            int ret = ::setsockopt(
                acpepetFd,
                SOL_SOCKET,
                SO_UPDATE_ACCEPT_CONTEXT,
                (char *)&activePendingReq->IoFd,
                sizeof(activePendingReq->IoFd)
            );

            set_active(iocp_event_fd::EV_ACCPET);
            handle_accpet_event(acpepetFd);
           
            enable_accpet(error);

            return 0;
        }

        int iocp_event_fd::handle_write_done()
        {
            errors::error_code error;

            assert(activePendingReq != nullptr);
            if (activePendingReq->IoSize == 0) {
                handle_zero();
                return 0;
            }
            assert(activePendingReq != nullptr);
            activePendingReq->IoOpt = iocp_event_fd::EV_WRITE;
            activePendingReq->SentBytes += activePendingReq->IoSize;
            if (activePendingReq->SentBytes < activePendingReq->TotalBytes) {
                post_write(activePendingReq->Buffer + activePendingReq->SentBytes,
                    activePendingReq->TotalBytes - activePendingReq->SentBytes, error);
            }
            else {
                set_active(iocp_event_fd::EV_WRITE);
                event_fd::handle_event();
            }
            return 0;
        }





        void iocp_event_fd::handle_event()
        {
            if (activePendingReq->IoOpt & iocp_event_fd::EV_WAKEUP) {
                //handleWakeUp();
                return ;
            }
            else if (activePendingReq->IoOpt & iocp_event_fd::EV_READ) {
                handle_read_done();
            }
            else if (activePendingReq->IoOpt & iocp_event_fd::EV_ACCPET) {
                handle_accept_done();
            }
            else if (activePendingReq->IoOpt & iocp_event_fd::EV_WRITE) {
                handle_write_done();
            }
        }

        int iocp_event_fd::post_write(const void *data, int len, errors::error_code &error)
        {
            DWORD sentBytes = 0;

            io_request_ref request = create_io_request(iocp_event_fd::EV_WRITE);
            memcpy(request.get()->Wsabuf.buf, data, len);
            request.get()->Wsabuf.len = len;
            request.get()->TotalBytes = len;

            int ret = WSASend(fd_, &request.get()->Wsabuf,
                1, &sentBytes, 0, &request.get()->Overlapped, NULL);
            if (!SUCCEEDED_WITH_IOCP(ret == 0)) {
                error = hht_make_error_code(static_cast<std::errc>(::WSAGetLastError()));
                return -1;
            }
            queued_pending_request(request);
            return 0;
        }

        static LPFN_ACCEPTEX                acceptEx = NULL;

        class IocpAccpeterFuncGetter {
        public:
            IocpAccpeterFuncGetter(int fd) {
                int ret = 0;
                DWORD                               bytes = 0;
                GUID                                acceptex_guid = WSAID_ACCEPTEX;
                system::call_once(flag, [&] {
                    ret = WSAIoctl(
                        fd,
                        SIO_GET_EXTENSION_FUNCTION_POINTER,
                        &acceptex_guid,
                        sizeof(acceptex_guid),
                        &acceptEx,
                        sizeof(acceptEx),
                        &bytes,
                        NULL,
                        NULL
                    );
                });
            }
        private:
            system::once_flag            flag;
        };

        
        iocp_event_fd::io_request_ref iocp_event_fd::create_io_request(int ev)
        {
            io_request_ref    request = std::make_shared<struct io_request_t>();

            request->IoFd = fd();
            request->IoOpt |= ev;
            return request;
        }

        iocp_event_fd::io_request_ref iocp_event_fd::remove_active_request()
        {
            io_request_ref active;
            active.swap(activePendingReq);
            return active;
        }
        void iocp_event_fd::set_active_pending(io_request_t *active)
        {
            auto find = ioRequestList.find(active);
            assert(find != ioRequestList.end());
            activePendingReq = find->second;
            ioRequestList.erase(find);
        }

        int iocp_event_fd::pending_request_size()
        {
            return  ioRequestList.size();
        }

        void iocp_event_fd::queued_pending_request(const io_request_ref &request)
        {
            ioRequestList[request.get()] = request;
        }


        int iocp_event_fd::post_accpet(errors::error_code &error)
        {
            DWORD   recvBytes = 0;
            int     ret = 0;
            static IocpAccpeterFuncGetter iocpAccpeterFuncGetter(fd());

            int accept_socket = net::NewSocket(AF_INET, SOCK_STREAM, error);
            hht_return_if_error(error, -1);

            io_request_ref    request = create_io_request(iocp_event_fd::EV_ACCPET);

            ret = acceptEx(fd(),
                accept_socket,
                (LPVOID)(request->Buffer),
                0,
                sizeof(SOCKADDR_STORAGE), 
                sizeof(SOCKADDR_STORAGE),
                &recvBytes,
                (LPOVERLAPPED) &(request->Overlapped));

            if (!SUCCEEDED_WITH_IOCP(ret)) {
                error = hht_make_error_code(static_cast<std::errc>(::WSAGetLastError()));
                closesocket(accept_socket);
                return -1;
            }
            queued_pending_request(request);
            request->AccpetFd = accept_socket;
            return 0;
        }



    }
}