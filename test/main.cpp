#include <errors/pp_error.h>
#include <io/io_event_loop.h>
#include <iostream>
#include <net/net_tcp_server.h>
#include <string>
#include <sync/sync_chan.h>
#include <time/_time.h>

#include <hht.h>

using namespace pp;

int main(int argc, char* argv)
{
    io::event_loop     loop;
    errors::error_code error;
    net::tcp_server    server(&loop, net::addr("0.0.0.0", 8080), "echo");

    server.new_connection([&](const net::tcp_conn_ref&  conn,
                              const _time::time&        time,
                              const errors::error_code& error) {
        errors::error_code err;
        if (!conn->connected()) {
            std::cout << "remote:" << conn->remote_addr(err).string()
                      << " closed "
                      << " " << error.message() << std::endl;
            return;
        }
        std::cout << time.string()
                  << " local:" << conn->local_addr(err).string()
                  << " remote:" << conn->remote_addr(err).string()
                  << " connected " << std::endl;
        conn->socket().set_tcp_nodelay(true, err);
		//conn->write("hello world\n", 12);
    });

    server.message_recved([&](const net::tcp_conn_ref& conn,
                              bytes::Buffer& message, const _time::time& time) {
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
        conn->write(resp.data(), resp.length());
        //conn->shutdown();
#if 0
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
                             // loop.quit();
                         });
#endif
    });

    _time::timer_ref timer;

    timer = _time::new_timer(_time::timer::interval, _time::Second * 1, [&]() {
        static int count = 0;
        if (++count == 5) {
            // timer->cancel();
            // loop.quit();
        }
        std::cout << "interval ticks " << count << std::endl;
    });
    _time::new_timer(_time::timer::oneshot, _time::Second * 1,
                     [&]() { std::cout << "oneshot timer" << std::endl; });

    server.listen_and_serv(error);
    loop.exec();
    return 0;
}
