#ifndef NET_TCP_CLIENT_H
#define NET_TCP_CLIENT_H

#include <container/any.h>
#include <errors/pp_error.h>
#include <net/net.h>
#include <net/net_tcp_conn.h>
#include <net/net_tcp_connector.h>
#include <string>
#include <time/_time.h>

namespace pp {
class io::event_loop;
}

namespace pp {
namespace net {
    class tcp_connector;
    class tcp_client {
    public:
        tcp_client(io::event_loop* loop, const addr& addr);
        ~tcp_client();

        void dial_done(const connection_handler& handler);
        void message_recved(const message_handler& handler);
        void data_write_finished(const write_finished_handler& handler);

        void dial();
        void write(char* data, int len);
        void close();
        void set_user_data(const pp::Any& any);

    private:
        void conn_connected(int fd, const errors::error_code& error);
        void conn_closed(const net::tcp_conn_ref&  conn,
                         const errors::error_code& error);

        //! Copy constructor
        tcp_client(const tcp_client& other);

        //! Move constructor
        tcp_client(tcp_client&& other) noexcept;

        //! Copy assignment operator
        tcp_client& operator=(const tcp_client& other);

        //! Move assignment operator
        tcp_client& operator=(tcp_client&& other) noexcept;

        io::event_loop*   loop_;
        tcp_connector_ref tcp_connector_;
        tcp_conn_ref      tcp_conn_;

        // only used for connect timeout
        _time::timer_ref connect_timer_;

        connection_handler     handle_connection_;
        message_handler        handle_recv_data_;
        write_finished_handler handle_write_finished_;
        addr                   addr_;
        pp::Any                any_;
    };
    typedef std::shared_ptr<tcp_client> tcp_client_ref;
}  // namespace net
}  // namespace pp

#endif
