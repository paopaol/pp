#ifndef NET_CONN_H
#define NET_CONN_H

#include <bytes/buffer.h>
#include <container/any.h>
#include <io/io_event_fd.h>
#include <net/net.h>
#include <time/_time.h>

#include <memory>

namespace pp {
namespace io {
    class io::event_loop;
}

namespace net {
    class tcp_conn;
    typedef std::shared_ptr<tcp_conn>                          tcp_conn_ref;
    typedef std::function<void(const net::tcp_conn_ref& conn)> error_handler;
    typedef std::function<void(const net::tcp_conn_ref&  conn,
                               const errors::error_code& error)>
                                                   close_handler;
    typedef std::function<void(int bytes_written)> write_finished_handler;

    typedef std::function<void(const net::tcp_conn_ref&, const _time::time&,
                               const errors::error_code& error)>
        connection_handler;
    typedef std::function<void(const net::tcp_conn_ref&, bytes::Buffer&,
                               const _time::time&)>
        message_handler;

    class tcp_conn : public std::enable_shared_from_this<tcp_conn> {
    public:
        enum ConnStatus { Connecting, Connected, DisConnecting, DisConnected };

        tcp_conn(io::event_loop* loop, int fd);
		~tcp_conn() {}
        void connected(const connection_handler& handler);
        void closed(const close_handler& handler);
        void data_recved(const message_handler& handler);
        void data_write_finished(const write_finished_handler& handler);

        void async_read();
        void write(const void* data, size_t len);
        void write(bytes::Buffer& buffer);

        int  close();
        void shutdown();
        void connect_established();
        void connect_destroyed(const errors::error_code& error);
        bool connected()
        {
            return state == Connected;
        }
        addr remote_addr(errors::error_code& error);
        addr local_addr(errors::error_code& error);
        void enable_read(errors::error_code& error);

        void set_user_data(const pp::Any& any)
        {
            user_data_ = any;
        }

        pp::Any user_data()
        {
            return user_data_;
        }

        net::socket& socket()
        {
            return socket_;
        }

    private:
        void read_done(errors::error_code& error);
        void write_done(errors::error_code& error);
        void handle_close(const errors::error_code& error);
        int  write(const void* data, size_t len, errors::error_code& error);
        void start_read(errors::error_code& error);
        void start_write(const void* data, size_t len,
                         errors::error_code& error);
        void write_some_buffer_data(errors::error_code& error);

        void shutdown_in_loop();

        DISABLE_COPY_CONSTRCT(tcp_conn);

        io::event_loop*        loop_;
        net::socket            socket_;
        io::event_fd_ref       event_fd_;
        message_handler        msg_read_handler_;
        message_handler        msg_write_handler_;
        close_handler          close_handler_;
        error_handler          error_handler_;
        connection_handler     connection_handler_;
        write_finished_handler write_finished_handler_;
        bytes::Buffer          read_buf_;
        bytes::Buffer          write_buf_;
        ConnStatus             state;
        addr                   remote_;
        addr                   local_;
        pp::Any                user_data_;
        errors::error_code     err_;
        int                    bytes_written_;
        bool                   pending_read_;
    };
}  // namespace net
}  // namespace pp

#endif
