#include <net/net_ip.h>
#include <net/net_tcp_connector.h>
#include <windows/io_win_iocp_event_fd.h>

#include <errors/pp_error.h>
#include <io/io_event_fd.h>
#include <io/io_event_loop.h>
#include <windows/errors_windows.h>
#include <windows/net_win.h>

#include <WinSock2.h>
#include <Windows.h>

using namespace std;

namespace pp {
namespace net {
    static LPFN_CONNECTEX connect_ex = nullptr;

    class windows_connect_ex_initer {
    public:
        windows_connect_ex_initer(int fd)
        {
            DWORD bytes           = 0;
            GUID  connect_ex_guid = WSAID_CONNECTEX;
            int   ret             = 0;

            system::call_once(flag, [&] {
                ret = WSAIoctl(fd, SIO_GET_EXTENSION_FUNCTION_POINTER,
                               &connect_ex_guid, sizeof(connect_ex_guid),
                               &connect_ex, sizeof(connect_ex), &bytes, 0, 0);

                {
                    assert(ret == 0
                           && "WSAIoctl(SIO_GET_EXTENSION_FUNCTION_POINTER)");
                }
            });
        }

    private:
        system::once_flag flag;
    };

    tcp_connector::tcp_connector(io::event_loop* loop, const addr& addr)
        : loop_(loop), addr_(addr)

    {
    }

    tcp_connector::~tcp_connector() {}

    void tcp_connector::set_new_conn_handler(const new_conn_handler& handler)
    {
        new_conn_handler_ = handler;
    }

    int tcp_connector::cancel_connect()
    {
        loop_->run_in_loop([&]() {
            assert(loop_->in_created_thread());

            auto evfd = static_cast<io::iocp_event_fd*>(conn_fd_.get());

            DWORD bytes;
            int   fd = evfd->fd();
            WSAIoctl(fd, SIO_BASE_HANDLE, NULL, 0, &fd, sizeof fd, &bytes, NULL,
                     NULL);
            CancelIo(( HANDLE )fd);
        });
        return 0;
    }

    int tcp_connector::start_connect(_time::Duration     timeout,
                                     errors::error_code& error)
    {
        sockaddr_in addr;
        sockaddr_in local;
        DWORD       send;
        int         ecode;

        assert(loop_->in_created_thread());

        int fd   = new_nonblock_socket(AF_INET, SOCK_STREAM, error);
        conn_fd_ = std::make_shared<io::iocp_event_fd>(loop_, fd);

        auto evfd = static_cast<io::iocp_event_fd*>(conn_fd_.get());
        static windows_connect_ex_initer ex_func_init(evfd->fd());
        evfd->tie(shared_from_this());

        net::ip4_addr("0.0.0.0", 0, &local);
        net::ip4_addr(addr_.ip.c_str(), addr_.port, &addr);

        ecode = ::bind(evfd->fd(), ( const sockaddr* )&local, sizeof local);
        if (ecode != 0) {
            make_win_socket_error_code(error, evfd->fd());
            return -1;
        }
        evfd->enable_connect(std::bind(&tcp_connector::connect_done, this),
                             error);

        auto request = evfd->create_io_request(io::iocp_event_fd::EV_CONNECT);
        int  ret =
            connect_ex(evfd->fd(), ( const sockaddr* )&addr, sizeof(addr), NULL,
                       0, &send, (LPOVERLAPPED) & (request->Overlapped));
        if (!SUCCEEDED_WITH_IOCP(ret, ecode)) {
            make_win_socket_error_code(error, evfd->fd());
            return -1;
        }

        evfd->queued_pending_request(request);

        if (timeout > 0) {
            timer_ = _time::new_timer(_time::timer::timer_behavior::oneshot,
                                      timeout, [&]() { cancel_connect(); });
        }
        return 0;
    }

    void tcp_connector::connect_done()
    {
        if (timer_) {
            timer_->cancel();
            timer_ = nullptr;
        }

        assert(loop_->in_created_thread());

        errors::error_code error;
        auto evfd = static_cast<io::iocp_event_fd*>(conn_fd_.get());
        int  fd   = evfd->fd();

        auto active = evfd->remove_active_request();

        // if not upadte fd of SO_UPDATE_CONNECT_CONTEXT,
        // then after getpeername will return 10057
        int ret =
            ::setsockopt(fd, SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, nullptr, 0);
        if (ret < 0) {
            make_win_socket_error_code(error, fd);
        }
        new_conn_handler_(fd, error);
    }

    void tcp_connector::detach_loop()
    {
        errors::error_code error;
        conn_fd_->remove_event(error);
    }

}  // namespace net
}  // namespace pp
