#include "windows/net_win_iocp_tcp_accpeter.h"

#include <io/io_event_fd.h>
#include <io/io_event_loop.h>

#include <WinSock2.h>
#include <Windows.h>

using namespace std;

namespace pp {
namespace net {
    static LPFN_ACCEPTEX acceptEx = NULL;

    class windows_ex_func_initer {
    public:
        windows_ex_func_initer(int fd)
        {
            int   ret           = 0;
            DWORD bytes         = 0;
            GUID  acceptex_guid = WSAID_ACCEPTEX;
            system::call_once(flag, [&] {
                ret = WSAIoctl(fd, SIO_GET_EXTENSION_FUNCTION_POINTER,
                               &acceptex_guid, sizeof(acceptex_guid), &acceptEx,
                               sizeof(acceptEx), &bytes, NULL, NULL);
                assert(ret == 0
                       && "WSAIoctl(SIO_GET_EXTENSION_FUNCTION_POINTER)");
            });
        }

    private:
        system::once_flag flag;
    };

    win_iocp_tcp_accpeter::win_iocp_tcp_accpeter(io::event_loop* loop)
        : loop_(loop), socket_(AF_INET, SOCK_STREAM,
                               newsocket(AF_INET, SOCK_STREAM, error_)),
          listen_fd_(std::make_shared<io::iocp_event_fd>(loop, socket_.fd()))
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
        auto ev_fd = static_cast<io::iocp_event_fd*>(listen_fd_.get());

        socket_.listen(error);
        ev_fd->enable_accpet(
            error,
            std::bind(&win_iocp_tcp_accpeter::start_accpet, this,
                      std::placeholders::_1),
            std::bind(&win_iocp_tcp_accpeter::accpet_done, this));
        return 0;
    }

    int win_iocp_tcp_accpeter::start_accpet(errors::error_code& error)
    {
        DWORD recvBytes = 0;
        int   ret       = 0;

        auto evfd = static_cast<io::iocp_event_fd*>(listen_fd_.get());
        static windows_ex_func_initer ex_func_init(evfd->fd());

        int accept_socket = net::newsocket(AF_INET, SOCK_STREAM, error);
        hht_return_if_error(error, -1);

        auto request = evfd->create_io_request(io::iocp_event_fd::EV_ACCPET);

        ret = acceptEx(evfd->fd(), accept_socket, (LPVOID)(request->Buffer), 0,
                       sizeof(SOCKADDR_STORAGE), sizeof(SOCKADDR_STORAGE),
                       &recvBytes, (LPOVERLAPPED) & (request->Overlapped));

        if (!SUCCEEDED_WITH_IOCP(ret)) {
            error = hht_make_error_code(
                static_cast<std::errc>(::WSAGetLastError()));
            closesocket(accept_socket);
            return -1;
        }
        evfd->queued_pending_request(request);
        request->AccpetFd = accept_socket;
        return 0;
    }

    void win_iocp_tcp_accpeter::accpet_done()
    {
        errors::error_code error;

        auto evfd = static_cast<io::iocp_event_fd*>(listen_fd_.get());

        auto active = evfd->remove_active_request();

        int client_fd = active->AccpetFd;
        int fd        = active->IoFd;

        int ret = ::setsockopt(client_fd, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
                               ( char* )&active->IoFd, sizeof(active->IoFd));
        evfd->enable_accpet(
            error,
            std::bind(&win_iocp_tcp_accpeter::start_accpet, this,
                      std::placeholders::_1),
            std::bind(&win_iocp_tcp_accpeter::accpet_done, this));

        handle_accpet_event(client_fd);
    }
}  // namespace net
}  // namespace pp
