#ifndef socket_iocp_accpeter_H
#define socket_iocp_accpeter_H
#include <net/net_socket_accpeter.h>

#include <errors/hht_error.h>
#include <hht.h>
#include <net/net.h>
#include <windows/io_win_iocp_event_fd.h>

namespace pp {
namespace net {
    class io::event_loop;
    class win_iocp_tcp_accpeter {
    public:
        win_iocp_tcp_accpeter(io::event_loop* loop);
        void set_new_conn_handler(const new_conn_handler& handler);
        int  bind(const addr& addr, errors::error_code& error);
        int  listen(errors::error_code& error);
        int  start_accpet(errors::error_code& error);

    private:
        void accpet_done();
        void handle_accpet_event(int fd);

        DISABLE_COPY_CONSTRCT(win_iocp_tcp_accpeter);

        io::event_loop*    loop_;
        socket             socket_;
        io::event_fd_ref   listen_fd_;
        new_conn_handler   new_conn_handler_;
        errors::error_code error_;
        int                type_;
    };
}  // namespace net
}  // namespace pp

#endif
