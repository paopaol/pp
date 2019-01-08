#include <io/io_event_loop.h>
#include <net/net_tcp_conn.h>
#include <net/net_tcp_server.h>
#ifdef WIN32
#include <windows/net_win_iocp_tcp_accpeter.h>
#endif

namespace pp {
namespace net {
    struct tcp_server_accpeter {
    };

    tcp_server::tcp_server(io::event_loop* loop, const net::addr& addr,
                           const std::string& name)
        : loop_(loop),
#ifdef WIN32
          accpeter_(reinterpret_cast<tcp_server_accpeter*>(
              new win_iocp_tcp_accpeter(loop_))),
#endif
          bind_addr_(addr),
          server_name_(name)
    {
        win_iocp_tcp_accpeter* accpeter =
            reinterpret_cast<win_iocp_tcp_accpeter*>(accpeter_);

        accpeter->set_new_conn_handler(
            [&](int fd, const errors::error_code& error) {
                on_new_conn(fd, error);
            });
    }

    void tcp_server::new_connection(const net::connection_handler& handler)
    {
        handle_new_conn_ = handler;
    }

    void tcp_server::message_recved(const net::message_handler& handler)
    {
        handle_recved_data = handler;
    }

    bool tcp_server::listen_and_serv(errors::error_code& error)
    {
        win_iocp_tcp_accpeter* accpeter =
            reinterpret_cast<win_iocp_tcp_accpeter*>(accpeter_);

        accpeter->bind(bind_addr_, error);
        hht_return_if_error(error, false);

        accpeter->listen(error);
        hht_return_if_error(error, false);
        return true;
    }

    void tcp_server::on_new_conn(int fd, const errors::error_code& error)
    {
        net::tcp_conn_ref conn =
            std::make_shared<net::tcp_conn>(loop_, fd, error);

        conn->connected(handle_new_conn_);
        conn->data_recved(handle_recved_data);
        conn->closed([&](const net::tcp_conn_ref&  conn,
                         const errors::error_code& error) {
            remove_from_conn_list(conn, error);
        });
        if (error.value() != 0) {
            handle_new_conn_(conn, _time::time(), error);
            return;
        }
        errors::error_code err;
        conn_list_[conn->remote_addr(err).string()] = conn;
        conn->connect_established(error);
    }

    void tcp_server::remove_from_conn_list(const net::tcp_conn_ref&  conn,
                                           const errors::error_code& error)
    {
        errors::error_code err_tmp;
        std::string        remote = conn->remote_addr(err_tmp).string();

        auto it = conn_list_.find(remote);
        if (it != conn_list_.end()) {
            conn_list_.erase(it);
            conn->connect_destroyed(error);
        }
    }
}  // namespace net
}  // namespace pp
