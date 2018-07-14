#include <io/io_event_loop.h>
#include <io/io_event_poller.h>
#include <net/net_socket_accpeter.h>
#include <net/net_tcp_conn.h>
#include <sync/sync_chan.h>
#include <time/_time.h>
#include <windows/net_win_iocp_tcp_accpeter.h>

#include <iostream>
#include <string>

#include <hht.h>

using namespace pp;

static std::map<net::tcp_conn_ref, net::tcp_conn_ref> connList;

class TcpServer {
public:
    TcpServer(io::event_loop* loop, const net::addr& addr,
              const std::string& name)
        : loop_(loop), accpeter_(loop), bindAddr_(addr), tcpServerName_(name)
    {
        accpeter_.set_new_conn_handler([&](int fd) { OnConnection(fd); });
    }

    void new_connection(const net::connection_handler& handler)
    {
        handle_new_conn = handler;
    }

    void message_recved(const net::message_handler& handler)
    {
        handle_recved_data = handler;
    }

    bool Start(errors::error_code& error)
    {
        accpeter_.bind(bindAddr_, error);
        hht_return_if_error(error, false);
        accpeter_.listen(error);
        hht_return_if_error(error, false);
        return true;
    }

private:
    void OnConnection(int fd)
    {
        errors::error_code error;

        net::tcp_conn_ref conn = std::make_shared<net::tcp_conn>(loop_, fd);
        connList_[conn->remote_addr(error).string()] = conn;

        conn->connected(handle_new_conn);
        conn->data_recved(handle_recved_data);
        conn->disconnected(
            [&](const net::tcp_conn_ref& conn) { removeFromConnList(conn); });
        conn->connect_established();
    }

    void removeFromConnList(const net::tcp_conn_ref& conn)
    {
        errors::error_code error;
        std::string        remote = conn->remote_addr(error).string();

        auto it = connList_.find(remote);
        if (it != connList_.end()) {
            connList_.erase(it);
            conn->connect_destroyed();
        }
        int i = conn.use_count();
    }

    io::event_loop*                          loop_;
    net::win_iocp_tcp_accpeter               accpeter_;
    std::map<std::string, net::tcp_conn_ref> connList_;
    std::string                              tcpServerName_;
    net::addr                                bindAddr_;

    net::connection_handler handle_new_conn;
    net::message_handler    handle_recved_data;
};

int main(int argc, char* argv)
{
    io::event_loop     loop;
    errors::error_code error;
    TcpServer          server(&loop, net::addr("0.0.0.0", 8080), "echo");

    server.new_connection(
        [&](const net::tcp_conn_ref& conn, const _time::Time& time) {
            errors::error_code error;
            if (!conn->connected()) {
                std::cout << "remote:" << conn->remote_addr(error).string()
                          << " closed" << std::endl;
                return;
            }

            std::cout << time.String()
                      << "  remote:" << conn->remote_addr(error).string()
                      << "connected" << std::endl;
        });

    server.message_recved([&](const net::tcp_conn_ref& conn,
                              bytes::Buffer& message, const _time::Time& time) {
        Slice s;
        message.Read(s);
        std::string str(s.data(), s.size());
        if (str == "quit\r\n") {
            loop.quit();
        }
        // std::cout << str << std::endl;
        std::string resp = "HTTP / 1.1 200 OK\nDate:Sat, 31 Dec 2005 23:59:59 "
                           "GMT\nContent-Type:text/html; "
                           "charset=ISO-8859-1\n\n<html><head><title>Wrox "
                           "Homepage</title></head><body><!--body goes here "
                           "--><p>hello pp</p></body></html>\n";

        for (int i = 1; i < 4; i++) {
            std::cout << "install timer" << i << std::endl;
            _time::new_timer(_time::timer::oneshot, _time::Second * 5 * i,
                             [&, i, conn, resp]() {
                                 std::cout << i << "seconds timeout"
                                           << std::endl;
                                 conn->write(resp.data(), resp.length());
                             });
        }

        _time::new_timer(_time::timer::oneshot, _time::Second * 20,
                         [&, conn]() {
                             std::cout << 20 << "close client" << std::endl;
                             conn->shutdown();
                             loop.quit();
                         });
    });

    _time::timer_ref timer;

    timer = _time::new_timer(_time::timer::interval, _time::Second * 1, [&]() {
        std::cout << "interval ticks" << std::endl;
        static int count = 0;
        if (++count == 5) {
            timer->cancel();
            //loop.quit();
        }
    });
    _time::new_timer(_time::timer::oneshot, _time::Second * 1,
                     [&]() { std::cout << "oneshot timer" << std::endl; });

    server.Start(error);
    loop.exec();
    // t.join();
    return 0;
}
