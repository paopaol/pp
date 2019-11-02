#ifndef SOCKET_WIN_H
#define SOCKET_WIN_H

#include <net/net.h>

#include <Windows.h>
#include <Ws2tcpip.h>
#include <strsafe.h>
#include <winsock2.h>

#include <errors/hht_error.h>
#include <errors/pp_error.h>
#include <stdio.h>
#include <stdlib.h>
#include <windows/errors_windows.h>

//#include <filesystem>

using namespace std;

namespace pp {
namespace net {

class WsasocketIniter {
public:
  WsasocketIniter() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
  }
  ~WsasocketIniter() { WSACleanup(); }
};

static WsasocketIniter wsa_socket_initer;

static int set_noblock(int fd, errors::error_code &error) {
  unsigned long set = 1;
  int ret = ::ioctlsocket(fd, FIONBIO, &set);
  if (ret != 0) {
    error = hht_make_error_code(
        static_cast<errors::error>(errors::error::NET_ERROR));
    error.suffix_msg(errors::win_errstr(::WSAGetLastError()));
    return -1;
  }
  return 0;
}

socket::socket(int af, int type, int fd) : fd_(fd), af_(af), type_(type) {}

socket::~socket() { close(fd_); }

int socket::create(int af, int type, errors::error_code &error) {
  int fd =
      static_cast<int>(::WSASocket(af, type, 0, NULL, 0, WSA_FLAG_OVERLAPPED));
  if (fd == INVALID_SOCKET) {
    error = hht_make_error_code(
        static_cast<errors::error>(errors::error::NET_ERROR));
    error.suffix_msg(errors::win_errstr(::WSAGetLastError()));
    return -1;
  }
  return fd;
}

int socket::fd() { return fd_; }

int socket::set_tcp_nodelay(bool set, errors::error_code &error) {
  int _set = set;

  int ret = ::setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, (const char *)&_set,
                         sizeof(_set));
  if (ret != 0) {
    error = hht_make_error_code(static_cast<std::errc>(::WSAGetLastError()));
    return -1;
  }
  return 0;
}

int socket::set_reuse_addr(bool set, errors::error_code &error) {
  int _set = set;

  int ret = ::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, (const char *)&_set,
                         sizeof(_set));
  if (ret != 0) {
    error = hht_make_error_code(
        static_cast<errors::error>(errors::error::NET_ERROR));
    error.suffix_msg(errors::win_errstr(::WSAGetLastError()));
    return -1;
  }
  return 0;
}

int socket::set_reuse_port(bool set, errors::error_code &error) {
#ifdef SO_REUSEPORT
  int _set = set;

  int ret = ::setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT, (const char *)&_set,
                         sizeof(_set));
  if (ret != 0) {
    error = hht_make_error_code(
        static_cast<errors::error>(errors::error::NET_ERROR));
    error.suffix_msg(errors::win_errstr(::WSAGetLastError()));
    return -1;
  }
#endif
  return 0;
}

int socket::set_keep_alive(bool set, errors::error_code &error) {
  int _set = set;
  int ret = ::setsockopt(fd_, SOL_SOCKET, SO_KEEPALIVE, (const char *)&_set,
                         sizeof(_set));
  if (ret != 0) {
    error = hht_make_error_code(
        static_cast<errors::error>(errors::error::NET_ERROR));
    error.suffix_msg(errors::win_errstr(::WSAGetLastError()));
    return -1;
  }
  return 0;
}

int socket::set_nonblock(errors::error_code &error) {
  return set_noblock(fd_, error);
}

int socket::bind(const addr &addr, errors::error_code &error) {
  struct sockaddr_in saddr;
  saddr.sin_family = af_;
  saddr.sin_port = htons(addr.port);
  if (::inet_pton(af_, addr.ip.c_str(), &saddr.sin_addr) <= 0) {
    error = hht_make_error_code(
        static_cast<errors::error>(errors::error::NET_ERROR));
    error.suffix_msg(errors::win_errstr(::WSAGetLastError()));
    return -1;
  }
  int ret = ::bind(fd_, (const struct sockaddr *)&saddr,
                   static_cast<socklen_t>(sizeof(struct sockaddr_in)));
  return 0;
}

int socket::listen(errors::error_code &error) {
  int ret = ::listen(fd_, 5);
  if (ret < 0) {
    error = hht_make_error_code(
        static_cast<errors::error>(errors::error::NET_ERROR));
    error.suffix_msg(errors::win_errstr(::WSAGetLastError()));
  }
  return ret;
}

int socket::shutdown_write(int fd) {
  shutdown((SOCKET)fd, SD_SEND);
  return 0;
}

int socket::close(int fd) {
  closesocket((SOCKET)fd);
  return 0;
}

addr socket::remote_addr(errors::error_code &error) {
  addr addr;

  struct sockaddr_in sa[2];
  int len = sizeof(sa);
  if (getpeername(fd_, (struct sockaddr *)&sa, &len) == 0) {
    addr.ip = inet_ntoa(sa[0].sin_addr);
    addr.port = ntohs(sa[0].sin_port);
  } else {
    int e = ::WSAGetLastError();
    error = hht_make_error_code(
        static_cast<errors::error>(errors::error::NET_ERROR));
    error.suffix_msg(errors::win_errstr(e));
  }
  return addr;
}

addr socket::local_addr(errors::error_code &error) {
  addr addr;
  struct sockaddr_in sa[2];
  int len = sizeof(sa);
  if (getsockname(fd_, (struct sockaddr *)&sa, &len) == 0) {
    addr.ip = inet_ntoa(sa[0].sin_addr);
    addr.port = ntohs(sa[0].sin_port);
  } else {
    error = hht_make_error_code(
        static_cast<errors::error>(errors::error::NET_ERROR));
    error.suffix_msg(errors::win_errstr(::WSAGetLastError()));
  }
  return addr;
}

int new_nonblock_socket(int af, int type, errors::error_code &error) {
  int fd = socket::create(af, type, error);
  set_noblock(fd, error);

  return fd;
}

} // namespace net
} // namespace pp

#endif
