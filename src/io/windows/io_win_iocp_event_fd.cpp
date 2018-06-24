
#ifdef WIN32
#include "windows/io_win_iocp_event_fd.h"
#endif

#include <io/io_event_loop.h>

#include <memory>
#include <net/net.h>
#include <system/sys_mutex.h>

using namespace std;



namespace pp {
namespace io {

    iocp_event_fd::iocp_event_fd(event_loop* loop, int fd) : event_fd(loop, fd)
    {
    }

    void iocp_event_fd::enable_accpet(errors::error_code &error, 
        const accpet_done_handler &handler)
    {
        enabled_event_ |= iocp_event_fd::EV_ACCPET;
        event_loop_->update_event_fd(this, error);
        accpet_event_handler_ = handler;
    }

    void iocp_event_fd::enable_read(errors::error_code& error,
        const start_read_handler &read_handler)
    {
        start_read_ = read_handler;
        event_fd::enable_read(error);
    }

    void iocp_event_fd::enable_write(errors::error_code& error,
        const start_write_handler &write_handler)
    {
        start_write_ = write_handler;
    }


#if 0
    void iocp_event_fd::enable_wakeup(errors::error_code& error)
    {
        enabled_event_ |= iocp_event_fd::EV_WAKEUP;
        event_loop_->update_event_fd(this, error);
    }
#endif





    void iocp_event_fd::post_read(errors::error_code& error)
    {
        if (start_read_) {
            start_read_(error);
         }
    }

    int iocp_event_fd::handle_zero_done()
    {
        set_active(iocp_event_fd::EV_CLOSE);
        event_fd::handle_event();
        return 0;
    }

    int iocp_event_fd::handle_read_done()
    {
        assert(active_pending_req_ != nullptr);

        if (active_pending_req_->IoSize == 0) {
            handle_zero_done();
            return 0;
        }
        set_active(iocp_event_fd::EV_READ);
        event_fd::handle_event();
        return 0;
    }

    int iocp_event_fd::handle_accept_done()
    {
        set_active(iocp_event_fd::EV_ACCPET);

        if (accpet_event_handler_) {
            accpet_event_handler_();
        }
        return 0;
    }

    int iocp_event_fd::handle_write_done()
    {
        errors::error_code error;

        assert(active_pending_req_ != nullptr);
        if (active_pending_req_->IoSize == 0) {
            handle_zero_done();
            return 0;
        }
        assert(active_pending_req_ != nullptr);
        active_pending_req_->IoOpt = iocp_event_fd::EV_WRITE;
        active_pending_req_->SentBytes += active_pending_req_->IoSize;
        if (active_pending_req_->SentBytes < active_pending_req_->TotalBytes) {
            //FIXME:need test
            const char *data = active_pending_req_->Buffer
                + active_pending_req_->SentBytes;
            int len = active_pending_req_->TotalBytes
                - active_pending_req_->SentBytes;
           post_write((const char *)data, len,
               std::bind(start_write_, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3), error);
        }
        else {
            set_active(iocp_event_fd::EV_WRITE);
            event_fd::handle_event();
        }
        return 0;
    }

    void iocp_event_fd::handle_event()
    {
#if 0
        if (active_pending_req_->IoOpt & iocp_event_fd::EV_WAKEUP) {
            // handleWakeUp();
            return;
        }
        else
#endif
        if (active_pending_req_->IoOpt & iocp_event_fd::EV_READ) {
            handle_read_done();
        }
        else if (active_pending_req_->IoOpt & iocp_event_fd::EV_ACCPET) {
            handle_accept_done();
        }
        else if (active_pending_req_->IoOpt & iocp_event_fd::EV_WRITE) {
            handle_write_done();
        }
    }

    int iocp_event_fd::post_write(const char *data, int len,
        const start_write_handler &write_handler, errors::error_code &error)
    {
        if (write_handler) {
            write_handler(data, len, error);
          }
#if 0
        DWORD sentBytes = 0;

        io_request_ref request = create_io_request(iocp_event_fd::EV_WRITE);
        memcpy(request.get()->Wsabuf.buf, data, len);
        request.get()->Wsabuf.len = len;
        request.get()->TotalBytes = len;

        int ret = WSASend(fd_, &request.get()->Wsabuf, 1, &sentBytes, 0,
                          &request.get()->Overlapped, NULL);
        if (!SUCCEEDED_WITH_IOCP(ret == 0)) {
            error = hht_make_error_code(
                static_cast<std::errc>(::WSAGetLastError()));
            return -1;
        }
        queued_pending_request(request);
#endif
        return 0;
    }

    static LPFN_ACCEPTEX acceptEx = NULL;

    class IocpAccpeterFuncGetter {
    public:
        IocpAccpeterFuncGetter(int fd)
        {
            int   ret           = 0;
            DWORD bytes         = 0;
            GUID  acceptex_guid = WSAID_ACCEPTEX;
            system::call_once(flag, [&] {
                ret = WSAIoctl(fd, SIO_GET_EXTENSION_FUNCTION_POINTER,
                               &acceptex_guid, sizeof(acceptex_guid), &acceptEx,
                               sizeof(acceptEx), &bytes, NULL, NULL);
            });
        }

    private:
        system::once_flag flag;
    };

    iocp_event_fd::io_request_ref iocp_event_fd::create_io_request(int ev)
    {
        io_request_ref request = std::make_shared<struct io_request_t>();

        request->IoFd = fd();
        request->IoOpt |= ev;
        return request;
    }

    iocp_event_fd::io_request_ref iocp_event_fd::remove_active_request()
    {
        io_request_ref active;
        active.swap(active_pending_req_);
        return active;
    }
    void iocp_event_fd::set_active_pending(io_request_t* active)
    {
        auto find = io_request_list_.find(active);
        assert(find != io_request_list_.end());
        active_pending_req_ = find->second;
        io_request_list_.erase(find);
    }

    int iocp_event_fd::pending_request_size()
    {
        return io_request_list_.size();
    }

    void iocp_event_fd::queued_pending_request(const io_request_ref& request)
    {
        io_request_list_[request.get()] = request;
    }

    int iocp_event_fd::post_accpet(errors::error_code& error)
    {
        DWORD                         recvBytes = 0;
        int                           ret       = 0;
        static IocpAccpeterFuncGetter iocpAccpeterFuncGetter(fd());

        int accept_socket = net::newsocket(AF_INET, SOCK_STREAM, error);
        hht_return_if_error(error, -1);

        io_request_ref request = create_io_request(iocp_event_fd::EV_ACCPET);

        ret = acceptEx(fd(), accept_socket, (LPVOID)(request->Buffer), 0,
                       sizeof(SOCKADDR_STORAGE), sizeof(SOCKADDR_STORAGE),
                       &recvBytes, (LPOVERLAPPED) & (request->Overlapped));

        if (!SUCCEEDED_WITH_IOCP(ret)) {
            error = hht_make_error_code(
                static_cast<std::errc>(::WSAGetLastError()));
            closesocket(accept_socket);
            return -1;
        }
        queued_pending_request(request);
        request->AccpetFd = accept_socket;
        return 0;
    }
}
}
