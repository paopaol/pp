#ifndef NET_CONN_H
#define NET_CONN_H

#include <bytes/buffer.h>
#include <io/io_event_fd.h>
#include <net/net.h>
#include <time/_time.h>

#include <memory>

namespace pp {
namespace net {
    class tcp_conn;
    typedef std::shared_ptr<tcp_conn> tcp_conn_ref;
    typedef std::function<void(const net::tcp_conn_ref& conn)> error_handler;
    typedef std::function<void(const net::tcp_conn_ref& conn)> close_handler;

    typedef std::function<void(const net::tcp_conn_ref, const _time::Time&)>
        connection_handler;
    typedef std::function<void(const net::tcp_conn_ref, bytes::Buffer&,
                               const _time::Time&)>
        message_handler;

    class io::event_loop;
    class tcp_conn : public std::enable_shared_from_this<tcp_conn> {
    public:
        enum ConnStatus { Connecting, Connected, DisConnecting, DisConnected };

        tcp_conn(io::event_loop* loop, int fd);
        ~tcp_conn()
        {
        }
        void set_connect_handler(const connection_handler& handler);
        void set_read_handler(const message_handler& handler);
        void set_write_handler(const message_handler& handler);
        // void set_error_handler(const error_handler &handler);
        void set_close_handler(const close_handler& handler);

        void write(const void* data, int len);
        void write(bytes::Buffer& buffer);

        int  close();
        void shutdown();
        void connect_established();
        void connect_destroyed();
        bool connected()
        {
            return state == Connected;
        }
        addr remote_addr(errors::error_code& error);
        addr local_addr(errors::error_code& error);
        void enable_read(errors::error_code& error);

    private:
        void handle_read(errors::error_code& error);
        void handle_write(errors::error_code& error);
        void handle_close(const errors::error_code& error);
        void handle_error(const errors::error_code& error);
        int write(const void* data, int len, errors::error_code& error);
        void start_read(errors::error_code &error);
        void start_write(const void* data, int len, errors::error_code &error);

        void shutdown_in_loop();

        DISABLE_COPY_CONSTRCT(tcp_conn);

        io::event_loop*    loop_;
        net::socket        socket_;
        io::event_fd_ref   event_fd_;
        message_handler    msg_read_handler_;
        message_handler    msg_write_handler_;
        close_handler      close_handler_;
        error_handler      error_handler_;
        connection_handler connection_handler_;
        bytes::Buffer      read_buf_;
        bytes::Buffer      write_buf_;
        ConnStatus         state;
        addr               remote_;
        addr               local_;
    };
}
}

#endif
