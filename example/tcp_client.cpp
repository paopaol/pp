
#include <errors/pp_error.h>
#include <io/io_event_loop.h>
#include <iostream>
#include <net/net_tcp_client.h>
#include <net/net_tcp_server.h>
#include <string>
#include <sync/sync_chan.h>
#include <time/_time.h>

#include <hht.h>

using namespace pp;

int main(int argc, char *argv[]) {
  errors::error_code error;
  io::event_loop loop;
  net::tcp_client client(&loop, net::addr("127.0.0.1", 9999));

  client.dial_done([&](const net::tcp_conn_ref &conn, const _time::time &now,
                       const errors::error_code &error) {
    if (!conn->connected() || error.value() != 0) {
      std::cout << "closed:" << error.message() << std::endl;
      return;
    }
    errors::error_code err;
    auto remote = conn->remote_addr(err);
    std::cout << "connected to[" << remote.ip << ":" << remote.port << "]"
              << std::endl;
    conn->async_read();
  });
  client.message_recved([&](const net::tcp_conn_ref &conn,
                            bytes::Buffer &buffer, const _time::time &now) {
    std::vector<char> msg;
    buffer.Read(msg);
    std::string msg_str(msg.data(), msg.size());
    std::cout << msg_str << std::endl;
    conn->write(msg.data(), msg.size());
  });

  client.dial();
  loop.exec();
  return 0;
}
