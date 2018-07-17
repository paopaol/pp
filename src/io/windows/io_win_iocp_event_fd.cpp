
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

    void iocp_event_fd::enable_accpet(errors::error_code&         error,
                                      const start_accpet_handler& start_handler,
                                      const accpet_done_handler&  done_handler)
    {
        enabled_event_ |= iocp_event_fd::EV_ACCPET;
        start_accpet_       = start_handler;
        handle_accpet_done_ = done_handler;
        event_loop_->update_event_fd(this, error);
    }

    void iocp_event_fd::enable_read(errors::error_code&       error,
                                    const start_read_handler& read_handler)
    {
        start_read_ = read_handler;
        event_fd::enable_read(error);
    }

    void iocp_event_fd::enable_write(errors::error_code&        error,
                                     const start_write_handler& write_handler)
    {
        start_write_ = write_handler;
    }

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

        if (active_pending_req_->io_size == 0) {
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
        errors::error_code error;

        if (handle_accpet_done_) {
            handle_accpet_done_(error);
        }
        return 0;
    }

    int iocp_event_fd::handle_write_done()
    {
        errors::error_code error;

        assert(active_pending_req_ != nullptr);
        if (active_pending_req_->io_size == 0) {
            handle_zero_done();
            return 0;
        }
        active_pending_req_->IoOpt = iocp_event_fd::EV_WRITE;
        active_pending_req_->sent_bytes += active_pending_req_->io_size;
        if (active_pending_req_->sent_bytes
            < active_pending_req_->total_bytes) {
            // FIXME:need test
            const char* data =
                active_pending_req_->buffer + active_pending_req_->sent_bytes;
            int len = active_pending_req_->total_bytes
                      - active_pending_req_->sent_bytes;
            post_write(( const char* )data, len,
                       std::bind(start_write_, std::placeholders::_1,
                                 std::placeholders::_2, std::placeholders::_3),
                       error);
        }
        else {
            set_active(iocp_event_fd::EV_WRITE);
            event_fd::handle_event();
        }
        return 0;
    }

    void iocp_event_fd::handle_event_with_guard()
    {
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

    void iocp_event_fd::handle_event()
    {
        std::shared_ptr<void> guard;

        if (tied_) {
            guard = tie_.lock();
            if (guard) {
                handle_event_with_guard();
            }
        }
        else {
            handle_event_with_guard();
        }
    }

    int iocp_event_fd::post_write(const char* data, int len,
                                  const start_write_handler& write_handler,
                                  errors::error_code&        error)
    {
        if (write_handler) {
            write_handler(data, len, error);
        }
        return 0;
    }

    iocp_event_fd::io_request_ref iocp_event_fd::create_io_request(int ev)
    {
        io_request_ref request = std::make_shared<struct io_request_t>();

        request->io_fd = fd();
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
        if (start_accpet_) {
            start_accpet_(error);
        }
        return 0;
    }
}  // namespace io
}  // namespace pp
