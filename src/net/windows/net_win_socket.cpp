#ifndef SOCKET_WIN_H
#define SOCKET_WIN_H

#include <net/net.h>

#include <winsock2.h>
#include <Windows.h>
#include <Ws2tcpip.h>
#include <strsafe.h>

#include <stdio.h>
#include <stdlib.h>

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
        };

        static  WsasocketIniter wsa_socket_initer;




        socket::socket(int af, int type, int fd)
            :fd_(fd)
            ,af_(af)
            ,type_(type)
        {
        }


        socket::~socket()
        {
            shutdown(fd_, SD_SEND);
        }
        int socket::create(int af, int type, errors::error_code &error)
        {
            int fd = ::WSASocket(af, type, 0,
                NULL, 0, WSA_FLAG_OVERLAPPED);
            if (fd == INVALID_SOCKET) {
				error = hht_make_error_code(static_cast<std::errc>(::WSAGetLastError()));
                return -1;
            }
            return fd;
        }



        int socket::fd()
        {
            return fd_;
        }

        int socket::set_tcp_nodelay(bool set, errors::error_code &error)
        {
            int _set = set;

            int ret = ::setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY,
                (const char *)&_set, sizeof(_set));
            if (ret != 0) {
				error = hht_make_error_code(static_cast<std::errc>(::WSAGetLastError()));
                return -1;
            }
            return 0;
        }

        int socket::set_reuse_addr(bool set, errors::error_code &error)
        {
            int _set = set;

            int ret = ::setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR,
                (const char *)&_set, sizeof(_set));
            if (ret != 0) {
				error = hht_make_error_code(static_cast<std::errc>(::WSAGetLastError()));
                return -1;
            }
            return 0;
        }

        int socket::set_reuse_port(bool set, errors::error_code &error)
        {
#ifdef SO_REUSEPORT
            int _set = set;

            int ret = ::setsockopt(fd_, SOL_SOCKET, SO_REUSEPORT,
                (const char *)&_set, sizeof(_set));
            if (ret != 0) {
                error = hht_make_error_code(static_cast<std::errc>(::WSAGetLastError()));
                return -1;
            }
#endif
            return 0;
        }

        int socket::set_keep_alive(bool set, errors::error_code &error)
        {
            int _set = set;
            int ret = ::setsockopt(fd_, SOL_SOCKET, SO_KEEPALIVE, (const char *)&_set, sizeof(_set));
            if (ret != 0) {
                error = hht_make_error_code(static_cast<std::errc>(::WSAGetLastError()));
                return -1;
            }
            return 0;
        }


        int socket::set_nonblock(errors::error_code &error)
        {
            unsigned long set = 1;
            int ret = ::ioctlsocket(fd_, FIONBIO, &set);
            if (ret != 0) {
				error = hht_make_error_code(static_cast<std::errc>(::WSAGetLastError()));
                return -1;
            }
            return 0;
        }

        int socket::bind(const addr &addr, errors::error_code &error)
        {
            struct sockaddr_in saddr;
            saddr.sin_family = af_;
            saddr.sin_port = htons(addr.Port);
            if (::inet_pton(af_, addr.Ip.c_str(), &saddr.sin_addr) <= 0) {
				error = hht_make_error_code(static_cast<std::errc>(::WSAGetLastError()));
                return -1;
            }
            int ret = ::bind(fd_, (const struct sockaddr *)&saddr,
                static_cast<socklen_t>(sizeof(struct sockaddr_in)));
            return 0;
        }

        int socket::listen(errors::error_code &error)
        {
            int ret = ::listen(fd_, SOMAXCONN);
            if (ret < 0) {
				error = hht_make_error_code(static_cast<std::errc>(::WSAGetLastError()));
            }
            return ret;
        }

        int socket::shutdown_write(int fd)
        {
            shutdown((SOCKET)fd, SD_SEND);
            return 0;
        }


        addr socket::remote_addr(errors::error_code &error)
        {
            addr addr;

            struct sockaddr_in sa;
            int len = sizeof(sa);
            if (getpeername(fd_, (struct sockaddr *)&sa, &len) == 0) {
                addr.Ip = inet_ntoa(sa.sin_addr);
                addr.Port = ntohs(sa.sin_port);
            }
            else {
				error = hht_make_error_code(static_cast<std::errc>(::WSAGetLastError()));
            }
            return addr;   
        }

        addr socket::local_addr(errors::error_code &error)
        {
            addr addr;
            struct sockaddr_in sa;
            int len = sizeof(sa);
            if (getsockname(fd_, (struct sockaddr *)&sa, &len) == 0) {
                addr.Ip = inet_ntoa(sa.sin_addr);
                addr.Port = ntohs(sa.sin_port);
            }
            else {
				error = hht_make_error_code(static_cast<std::errc>(::WSAGetLastError()));
            }
            return addr;
        }

        int newsocket(int af, int type, errors::error_code &error)
        {
            int fd = socket::create(af, type, error);

            return fd;
        }

    }
}

#endif