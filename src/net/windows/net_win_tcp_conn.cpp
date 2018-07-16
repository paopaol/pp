#include <net/net_tcp_conn.h>

#include <bytes/buffer.h>
#include <fmt/fmt.h>
#include <hht.h>
#include <io/io_event_loop.h>
#include <windows/io_win_iocp_event_fd.h>

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
          local_("", -1)
    {
        event_fd_->data_recved(
            [&](errors::error_code& error) { handle_read(error); });
        event_fd_->set_write_handler(
            [&](errors::error_code& error) { handle_write(error); });
        event_fd_->closed(
            [&](errors::error_code& error) { handle_close(error); });
        event_fd_->set_error_handler(
            [&](errors::error_code& error) { handle_error(error); });

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

    void tcp_conn::set_write_handler(const message_handler& handler)
    {
        msg_write_handler_ = handler;
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
            return;
        }

        assert(state == Connected || state == DisConnecting);
        state = DisConnected;

        if (close_handler_) {
            close_handler_(shared_from_this());
        }
    }

    void tcp_conn::handle_error(const errors::error_code& error)
    {
        io::iocp_event_fd* evfd =
            static_cast<io::iocp_event_fd*>(event_fd_.get());

        state = DisConnected;
        if (error_handler_ && evfd->pending_request_size() == 0) {
            error_handler_(shared_from_this());
        }
    }

    void tcp_conn::handle_read(errors::error_code& error)
    {
        io::iocp_event_fd* evfd =
            static_cast<io::iocp_event_fd*>(event_fd_.get());

        io::iocp_event_fd::io_request_ref done_req =
            evfd->remove_active_request();

        read_buf_.Write(( const char* )done_req->buffer, done_req->io_size);

        if (read_buf_.Len() > 0 && msg_read_handler_) {
            msg_read_handler_(shared_from_this(), read_buf_, _time::now());
        }

        if (evfd->pending_request_size() > 0) {
            return;
        }

        evfd->enable_read(error, std::bind(&tcp_conn::start_read, this,
                                           std::placeholders::_1));
        if (error.value() != 0) {
            handle_close(error);
        }
    }

    void tcp_conn::handle_write(errors::error_code& error)
    {
        io::iocp_event_fd* evfd =
            static_cast<io::iocp_event_fd*>(event_fd_.get());

        io::iocp_event_fd::io_request_ref unused =
            evfd->remove_active_request();

        // write remaining data
        if (write_buf_.Len() > 0) {
            int  min_write = min(write_buf_.Len(), MAX_WSA_BUFF_SIZE);
            char data[MAX_WSA_BUFF_SIZE] = { 0 };
            write_buf_.Read(data, sizeof(data));

            evfd->post_write(
                data, min_write,
                std::bind(&tcp_conn::start_write, this, std::placeholders::_1,
                          std::placeholders::_2, std::placeholders::_3),
                error);
            if (error.value() != 0) {
                handle_close(error);
            }
            return;
        }
        //���������Ͷ��һ����������
        //��ô�´ζԷ����͹��������ݾͲ�����յ���

        if (evfd->pending_request_size() > 0) {
            return;
        }
        evfd->enable_read(error, std::bind(&tcp_conn::start_read, this,
                                           std::placeholders::_1));
        if (error.value() != 0) {
            handle_close(error);
        }
    }

    void tcp_conn::write(const void* data, int len)
    {
        if (state == Connected) {
            Slice slice(( const char* )data, ( const char* )data + len);

            loop_->run_in_loop([&, slice]() {
                errors::error_code error;
                write(slice.data(), slice.size(), error);
            });
        }
    }

    int tcp_conn::write(const void* data, int len, errors::error_code& error)
    {
        if (state != Connected) {
            return 0;
        }

        assert(loop_->in_created_thread());

        io::iocp_event_fd* evfd =
            static_cast<io::iocp_event_fd*>(event_fd_.get());

        /*Conn �����,��һ�η������ݵĻ�,write_buf_
         *��ȻΪ�ա�HasPostedWrite��ȻΪfalse,���Կ���ֱ�ӷ��͸�socket������д�뻺��,
         *����Ѿ��������ύ��socket�Ļ�����ô���ϴ��ύ�����ݣ��ɹ�������֮ǰ������
         *�ٴ��ύ���ͣ���Ҫ�Ƚ��仺����������ΪfdCtx��д����ֻ��һ���������һ�η��͵����ݻ�
         *û�д����꣬�ͷ��͵ڶ��Σ���һ�ε�δ����������ݾͻᱻ���ǵ�.
         *
         * �ٶ���һ�η��͵����ݺܴ󣬲���һ���Է��͸�socket����ô����Ҫ�ȷ���һ���֣�ʣ�µ�
         *д�뻺��
         */

        if (write_buf_.Len() > 0 && evfd->pending_request_size() == 0) {
            write_buf_.Write(( char* )data, len);
            return 0;
        }

        if (len <= MAX_WSA_BUFF_SIZE) {
            evfd->post_write(
                ( const char* )data, len,
                std::bind(&tcp_conn::start_write, this, std::placeholders::_1,
                          std::placeholders::_2, std::placeholders::_3),
                error);
            return 0;
        }
        // data is too long, only write some
        evfd->post_write(( const char* )data, MAX_WSA_BUFF_SIZE,
                         std::bind(&tcp_conn::start_write, this,
                                   std::placeholders::_1, std::placeholders::_2,
                                   std::placeholders::_3),
                         error);
        write_buf_.Write(( char* )data + MAX_WSA_BUFF_SIZE,
                         len - MAX_WSA_BUFF_SIZE);
        return 0;
    }

    void tcp_conn::start_read(errors::error_code& error)
    {
        io::iocp_event_fd* evfd =
            static_cast<io::iocp_event_fd*>(event_fd_.get());

        DWORD recvBytes = 0;
        DWORD flags     = 0;

        io::iocp_event_fd::io_request_ref request =
            evfd->create_io_request(io::event_fd::EV_READ);
        int ret = WSARecv(evfd->fd(), &request.get()->wasbuf, 1, &recvBytes,
                          &flags, &request.get()->Overlapped, NULL);
        if (!SUCCEEDED_WITH_IOCP(ret == 0)) {
            int code = ::GetLastError();
            error    = hht_make_error_code(static_cast<std::errc>(code));
            printf("%s", error.full_message().c_str());
            return;
        }
        evfd->queued_pending_request(request);

        return;
    }

    void tcp_conn::start_write(const void* data, int len,
                               errors::error_code& error)
    {
        io::iocp_event_fd* evfd =
            static_cast<io::iocp_event_fd*>(event_fd_.get());

        DWORD sentBytes = 0;

        io::iocp_event_fd::io_request_ref request =
            evfd->create_io_request(io::iocp_event_fd::EV_WRITE);
        memcpy(request.get()->wasbuf.buf, data, len);
        request.get()->wasbuf.len = len;
        request.get()->total_bytes = len;

        int ret = WSASend(evfd->fd(), &request.get()->wasbuf, 1, &sentBytes, 0,
                          &request.get()->Overlapped, NULL);
        if (!SUCCEEDED_WITH_IOCP(ret == 0)) {
            error = hht_make_error_code(
                static_cast<std::errc>(::WSAGetLastError()));
            return;
        }
        evfd->queued_pending_request(request);
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
        if (!remote_.Ip.empty()) {
            return remote_;
        }

        remote_ = socket_.remote_addr(error);
        return remote_;
    }

    addr tcp_conn::local_addr(errors::error_code& error)
    {
        if (!local_.Ip.empty()) {
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
        if (connection_handler_) {
            connection_handler_(shared_from_this(), _time::time());
        }
        event_fd_->tie(shared_from_this());
        errors::error_code error;
        evfd->enable_read(error, std::bind(&tcp_conn::start_read, this,
                                           std::placeholders::_1));
        evfd->enable_write(error, std::bind(&tcp_conn::start_write, this,
                                            std::placeholders::_1,
                                            std::placeholders::_2,
                                            std::placeholders::_3));
        if (error.value() == 0) {
        }
    }

    void tcp_conn::connect_destroyed()
    {
        io::iocp_event_fd* evfd =
            static_cast<io::iocp_event_fd*>(event_fd_.get());

        assert(loop_->in_created_thread());

        if (state == DisConnected) {
            if (connection_handler_) {
                connection_handler_(shared_from_this(), _time::now());
            }
            errors::error_code error;
            evfd->remove_event(error);
        }
    }
}  // namespace net
}  // namespace pp
