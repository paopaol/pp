#include <net/net_tcp_conn.h>

#include <bytes/buffer.h>
#include <errors/pp_error.h>
#include <fmt/fmt.h>
#include <hht.h>
#include <io/io_event_loop.h>
#include <windows/io_win_iocp_event_fd.h>
#ifdef WIN32
#include <windows/errors_windows.h>
#endif

#include <cassert>
#include <memory>
using namespace std;

namespace pp {
namespace net {
    tcp_conn::tcp_conn(io::event_loop* loop, int fd)
        : loop_(loop),
          socket_(AF_INET, SOCK_STREAM, fd),
          event_fd_(std::make_shared<io::iocp_event_fd>(loop_, fd)),
          state(Connecting),
          remote_("", -1),
          local_("", -1),
          bytes_written_(0),
          pending_read_(false)
    {
        event_fd_->data_recved(
            [&](errors::error_code& error) { read_done(error); });
        event_fd_->set_write_handler(
            [&](errors::error_code& error) { write_done(error); });

        event_fd_->closed(
            [&](errors::error_code& error) { handle_close(error); });

        errors::error_code error;
        socket_.set_nonblock(error);
    }

    void tcp_conn::connected(const connection_handler& handler)
    {
        connection_handler_ = handler;
    }

    void tcp_conn::data_recved(const message_handler& handler)
    {
        msg_read_handler_ = handler;
    }

    void tcp_conn::data_write_finished(const write_finished_handler& handler)
    {
        write_finished_handler_ = handler;
    }

    void tcp_conn::closed(const close_handler& handler)
    {
        close_handler_ = handler;
    }

    void tcp_conn::handle_close(const errors::error_code& error)
    {
        io::iocp_event_fd* evfd =
            static_cast<io::iocp_event_fd*>(event_fd_.get());
        evfd->remove_active_request();
        if (evfd->pending_request_size() > 0) {
            err_ = error;
            return;
        }

        assert(state == Connected || state == DisConnecting);
        state = DisConnected;

        if (close_handler_) {
            close_handler_(shared_from_this(), err_);
        }
    }

    void tcp_conn::read_done(errors::error_code& error)
    {
        io::iocp_event_fd* evfd =
            static_cast<io::iocp_event_fd*>(event_fd_.get());

        io::iocp_event_fd::io_request_ref done_req =
            evfd->remove_active_request();

        pending_read_ = false;

        read_buf_.Write(( const char* )done_req->buffer, done_req->io_size);

        if (read_buf_.Len() > 0 && msg_read_handler_) {
            msg_read_handler_(shared_from_this(), read_buf_, _time::now());
        }

        // write_some_buffer_data(error);
        // if (error.value() != 0) {
        //   handle_close(error);
        //  return;
        //}

        // if (evfd->pending_request_size() > 0) {
        //     return;
        // }

        // if (error.value() != 0) {
        //     handle_close(error);
        // }
    }

    void tcp_conn::write_done(errors::error_code& error)
    {
        io::iocp_event_fd* evfd =
            static_cast<io::iocp_event_fd*>(event_fd_.get());

        io::iocp_event_fd::io_request_ref active =
            evfd->remove_active_request();
        assert(active);

        active->sent_bytes += active->io_size;
        if (active->sent_bytes < active->total_bytes) {
            // FIXME:need test
            const char* data = active->buffer + active->sent_bytes;
            int         len  = active->total_bytes - active->sent_bytes;

            start_write(data, len, error);
            if (error.value() != 0) {
                handle_close(error);
            }
            return;
        }

        bytes_written_ += active->sent_bytes;
        if (write_buf_.Len() == 0) {
            if (write_finished_handler_) {
                write_finished_handler_(bytes_written_);
            }
            bytes_written_ = 0;
        }

        // write remaining data
        // if has remaining data, this call will add
        // one io pending request
        write_some_buffer_data(error);
        if (error.value() != 0) {
            handle_close(error);
            return;
        }

        // if already have pending io request,
        // return
        if (evfd->pending_request_size() > 0) {
            return;
        }
        start_read(error);
        if (error.value() != 0) {
            handle_close(error);
        }
    }

    void tcp_conn::write(const void* data, size_t len)
    {
        if (state == Connected) {
            Slice slice(( const char* )data, ( const char* )data + len);

            loop_->run_in_loop([&, slice]() {
                errors::error_code error;
                write(slice.data(), slice.size(), error);
                if (error.value() != 0) {
                    handle_close(error);
                }
            });
        }
    }

    int tcp_conn::write(const void* data, size_t len, errors::error_code& error)
    {
        if (state != Connected) {
            return 0;
        }

        assert(loop_->in_created_thread());

        io::iocp_event_fd* evfd =
            static_cast<io::iocp_event_fd*>(event_fd_.get());

        /*Conn 构造后,第一次发送数据的话,write_buf_
         *必然为空。HasPostedWrite必然为false,所以可以直接发送给socket，无需写入缓存,
         *如果已经有数据提交给socket的话，那么在上次提交的数据，成功发送完之前，不能
         *再次提交发送，需要先将其缓存起来。因为fdCtx的写缓冲只有一个，如果第一次发送的数据还
         *没有处理完，就发送第二次，上一次的未处理完的数据就会被覆盖掉.
         *
         * 假定第一次发送的数据很大，不能一次性发送给socket，那么就需要先发送一部分，剩下的
         *写入缓冲
         */

        if (write_buf_.Len() > 0 && evfd->pending_request_size() == 0) {
            write_buf_.Write(( char* )data, static_cast<int>(len));
            return 0;
        }

        if (len <= MAX_WSA_BUFF_SIZE) {
            start_write(( const char* )data, len, error);
            return 0;
        }
        // data is too long, only write some
        start_write(( const char* )data, MAX_WSA_BUFF_SIZE, error);
        write_buf_.Write(( char* )data + MAX_WSA_BUFF_SIZE,
                         static_cast<int>(len - MAX_WSA_BUFF_SIZE));
        return 0;
    }

    void tcp_conn::start_read(errors::error_code& error)
    {
        io::iocp_event_fd* evfd =
            static_cast<io::iocp_event_fd*>(event_fd_.get());

        DWORD recvBytes = 0;
        DWORD flags     = 0;
        int   ecode     = 0;

        // if call start_read multi times, only one
        // realy post read
        if (pending_read_) {
            return;
        }

        io::iocp_event_fd::io_request_ref request =
            evfd->create_io_request(io::event_fd::EV_READ);
        int ret = WSARecv(evfd->fd(), &request.get()->wasbuf, 1, &recvBytes,
                          &flags, &request.get()->Overlapped, NULL);
        if (!SUCCEEDED_WITH_IOCP(ret == 0, ecode)) {
            error = hht_make_error_code(errors::error::NET_ERROR);
            error.suffix_msg(errors::win_errstr(ecode));
            return;
        }
        evfd->queued_pending_request(request);
        pending_read_ = true;
        return;
    }

    void tcp_conn::write_some_buffer_data(errors::error_code& error)
    {
        if (write_buf_.Len() > 0) {
            size_t min_write = min(write_buf_.Len(), MAX_WSA_BUFF_SIZE);
            char   data[MAX_WSA_BUFF_SIZE] = { 0 };
            write_buf_.Read(data, sizeof(data));
            write(data, min_write, error);
            return;
        }
    }

    void tcp_conn::start_write(const void* data, size_t len,
                               errors::error_code& error)
    {
        io::iocp_event_fd* evfd =
            static_cast<io::iocp_event_fd*>(event_fd_.get());

        DWORD sentBytes = 0;
        int   ecode     = 0;

        io::iocp_event_fd::io_request_ref request =
            evfd->create_io_request(io::iocp_event_fd::EV_WRITE);
        memcpy(request.get()->wasbuf.buf, data, len);
        request.get()->wasbuf.len  = static_cast<ULONG>(len);
        request.get()->total_bytes = static_cast<ULONG>(len);

        int ret = WSASend(evfd->fd(), &request.get()->wasbuf, 1, &sentBytes, 0,
                          &request.get()->Overlapped, NULL);
        if (!SUCCEEDED_WITH_IOCP(ret == 0, ecode)) {
            error = hht_make_error_code(errors::error::NET_ERROR);
            error.suffix_msg(errors::win_errstr(ecode));
            return;
        }

        evfd->queued_pending_request(request);
    }

    void tcp_conn::async_read()
    {
        loop_->run_in_loop([&]() {
            errors::error_code err;
            start_read(err);
        });
    }
    void tcp_conn::write(bytes::Buffer& buffer)
    {
        Slice slice;
        buffer.Read(slice);

        loop_->run_in_loop([&, slice]() {
            errors::error_code error;
            write(( const char* )slice.data(), slice.size(), error);
        });
    }

    int tcp_conn::close()
    {
        // state = DisConnecting;
        // CloseHandle((HANDLE)event_fd_->fd());
        // if (close_handler_) {
        //    close_handler_();
        //}

        return 0;
    }

    addr tcp_conn::remote_addr(errors::error_code& error)
    {
        if (!remote_.ip.empty()) {
            return remote_;
        }

        remote_ = socket_.remote_addr(error);
        return remote_;
    }

    addr tcp_conn::local_addr(errors::error_code& error)
    {
        if (!local_.ip.empty()) {
            return local_;
        }
        local_ = socket_.local_addr(error);
        return local_;
    }

    void tcp_conn::enable_read(errors::error_code& error)
    {
        event_fd_->enable_read(error);
    }

    void tcp_conn::shutdown()
    {
        if (state != Connected) {
            return;
        }
        state = DisConnecting;
        loop_->run_in_loop([&]() { shutdown_in_loop(); });
    }

    void tcp_conn::shutdown_in_loop()
    {
        io::iocp_event_fd* evfd =
            static_cast<io::iocp_event_fd*>(event_fd_.get());

        assert(loop_->in_created_thread());
        socket::shutdown_write(evfd->fd());
    }

    void tcp_conn::connect_established()
    {
        io::iocp_event_fd* evfd =
            static_cast<io::iocp_event_fd*>(event_fd_.get());

        assert(loop_->in_created_thread());
        assert(state == Connecting);
        state = Connected;

        event_fd_->tie(shared_from_this());
        errors::error_code error;

        // first, if no pending io request, we push one read request
        // at least, there was must one read request
        if (evfd->pending_request_size() == 0) {
            enable_read(error);
            // start_read(error);
            // if (error.value() != 0) {
            //    handle_close(error);
            //    return;
            //}
        }

        if (connection_handler_) {
            connection_handler_(shared_from_this(), _time::time(),
                                errors::error_code());
        }
    }

    void tcp_conn::connect_destroyed(const errors::error_code& error)
    {
        io::iocp_event_fd* evfd =
            static_cast<io::iocp_event_fd*>(event_fd_.get());

        assert(loop_->in_created_thread());

        if (state == DisConnected) {
            if (connection_handler_) {
                connection_handler_(shared_from_this(), _time::now(), error);
            }
            errors::error_code err;
            evfd->remove_event(err);
        }
    }
}  // namespace net
}  // namespace pp
