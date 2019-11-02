#include "windows/net_win_iocp_tcp_accpeter.h"
#include <windows/net_win.h>

#include <errors/pp_error.h>
#include <io/io_event_fd.h>
#include <io/io_event_loop.h>
#include <windows/errors_windows.h>

#include <WinSock2.h>
#include <Windows.h>

using namespace std;

namespace pp {
namespace net {
static LPFN_ACCEPTEX accpet_ex = NULL;

class windows_ex_func_initer {
public:
  windows_ex_func_initer(int fd) {
    int ret = 0;
    DWORD bytes = 0;
    GUID acceptex_guid = WSAID_ACCEPTEX;
    system::call_once(flag, [&] {
      ret = WSAIoctl(fd, SIO_GET_EXTENSION_FUNCTION_POINTER, &acceptex_guid,
                     sizeof(acceptex_guid), &accpet_ex, sizeof(accpet_ex),
                     &bytes, NULL, NULL);
      assert(ret == 0 && "WSAIoctl(SIO_GET_EXTENSION_FUNCTION_POINTER)");
    });
  }

private:
  system::once_flag flag;
};

win_iocp_tcp_accpeter::win_iocp_tcp_accpeter(io::event_loop *loop)
    : loop_(loop), socket_(AF_INET, SOCK_STREAM,
                           new_nonblock_socket(AF_INET, SOCK_STREAM, error_)),
      listen_fd_(std::make_shared<io::iocp_event_fd>(loop, socket_.fd())) {
  assert(error_.value() == 0);
  socket_.set_reuse_addr(true, error_);
  socket_.set_nonblock(error_);
}

void win_iocp_tcp_accpeter::set_new_conn_handler(
    const new_conn_handler &handler) {
  new_conn_handler_ = handler;
}

void win_iocp_tcp_accpeter::handle_accpet_done(
    int fd, const errors::error_code &error) {
  if (new_conn_handler_) {
    new_conn_handler_(fd, error);
  }
}

int win_iocp_tcp_accpeter::bind(const addr &addr, errors::error_code &error) {
  return socket_.bind(addr, error);
}

int win_iocp_tcp_accpeter::listen(errors::error_code &error) {
  auto ev_fd = static_cast<io::iocp_event_fd *>(listen_fd_.get());

  socket_.listen(error);
  ev_fd->enable_accpet(std::bind(&win_iocp_tcp_accpeter::accpet_done, this),
                       error);
  start_accpet(error);
  return 0;
}

int win_iocp_tcp_accpeter::start_accpet(errors::error_code &error) {
  DWORD recv_bytes = 0;
  int ret = 0;
  int ecode = 0;

  auto evfd = static_cast<io::iocp_event_fd *>(listen_fd_.get());
  static windows_ex_func_initer ex_func_init(evfd->fd());

  int accept_socket = net::new_nonblock_socket(AF_INET, SOCK_STREAM, error);
  hht_return_if_error(error, -1);

  auto request = evfd->create_io_request(io::iocp_event_fd::EV_ACCPET);

  ret = accpet_ex(evfd->fd(), accept_socket, (LPVOID)(request->buffer), 0,
                  sizeof(SOCKADDR_STORAGE), sizeof(SOCKADDR_STORAGE),
                  &recv_bytes, (LPOVERLAPPED) & (request->Overlapped));

  if (!SUCCEEDED_WITH_IOCP(ret, ecode)) {
    make_win_socket_error_code(error, accept_socket);
    closesocket(accept_socket);
    return -1;
  }
  evfd->queued_pending_request(request);
  request->accpet_fd = accept_socket;
  return 0;
}

void win_iocp_tcp_accpeter::accpet_done() {
  errors::error_code error;

  auto evfd = static_cast<io::iocp_event_fd *>(listen_fd_.get());

  auto active = evfd->remove_active_request();

  int client_fd = active->accpet_fd;
  int fd = active->io_fd;

  int ret = ::setsockopt(client_fd, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
                         (char *)&active->io_fd, sizeof(active->io_fd));
  if (ret < 0) {
    make_win_socket_error_code(error, client_fd);
    handle_accpet_done(client_fd, error);
    return;
  }

  evfd->enable_accpet(std::bind(&win_iocp_tcp_accpeter::accpet_done, this),
                      error);

  start_accpet(error);
  handle_accpet_done(client_fd, error);
}
} // namespace net
} // namespace pp
