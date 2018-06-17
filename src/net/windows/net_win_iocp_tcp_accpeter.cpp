#include "windows/net_win_iocp_tcp_accpeter.h"

#include <io/io_event_fd.h>
#include <io/io_event_loop.h>

#include <MSWSock.h>
#include <WinSock2.h>
#include <Windows.h>

using namespace std;

namespace pp {
namespace net {
    win_iocp_tcp_accpeter::win_iocp_tcp_accpeter(io::event_loop* loop)
        : loop_(loop), socket_(AF_INET, SOCK_STREAM,
                               newsocket(AF_INET, SOCK_STREAM, error_)),
          listen_fd_(loop, socket_.fd())
    {
        assert(error_.value() == 0);
        socket_.set_reuse_addr(true, error_);
        socket_.set_nonblock(error_);
    }

    void
    win_iocp_tcp_accpeter::set_new_conn_handler(const new_conn_handler& handler)
    {
        new_conn_handler_ = handler;
    }

    void win_iocp_tcp_accpeter::handle_accpet_event(int fd)
    {
        new_conn_handler_(fd);
    }

    int win_iocp_tcp_accpeter::bind(const addr& addr, errors::error_code& error)
    {
        return socket_.bind(addr, error);
    }

    int win_iocp_tcp_accpeter::listen(errors::error_code& error)
    {

        socket_.listen(error);
        listen_fd_.enable_accpet(
            error, std::bind(&win_iocp_tcp_accpeter::accpet_done, this));
        return 0;
    }

    void win_iocp_tcp_accpeter::accpet_done()
    {
        errors::error_code error;

        auto active = listen_fd_.remove_active_request();

        int client_fd = active->AccpetFd;
        int fd         = active->IoFd;

        int ret = ::setsockopt(client_fd, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
                               ( char* )&active->IoFd, sizeof(active->IoFd));
        listen_fd_.enable_accpet(
            error, std::bind(&win_iocp_tcp_accpeter::accpet_done, this));

        handle_accpet_event(client_fd);
    }
}  // namespace net
}  // namespace pp