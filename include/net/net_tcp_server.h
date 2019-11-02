#ifndef NET_TCP_SERVER_H
#define NET_TCP_SERVER_H

#include <map>
#include <net/net.h>
#include <net/net_socket_accpeter.h>
#include <net/net_tcp_conn.h>

namespace pp {
class io::event_loop;
}

namespace pp {
namespace net {
struct tcp_server_accpeter;
class tcp_server {
public:
  tcp_server(io::event_loop *loop, const addr &addr, const std::string &name);
  void new_connection(const net::connection_handler &handler);
  void message_recved(const net::message_handler &handler);
  bool listen_and_serv(errors::error_code &error);

private:
  void on_new_conn(int fd, const errors::error_code &error);
  void remove_from_conn_list(const net::tcp_conn_ref &conn,
                             const errors::error_code &error);

  io::event_loop *loop_;
  tcp_server_accpeter *accpeter_;
  std::map<std::string, net::tcp_conn_ref> conn_list_;
  std::string server_name_;
  addr bind_addr_;

  connection_handler handle_new_conn_;
  message_handler handle_recved_data;
};
} // namespace net
} // namespace pp

#endif
