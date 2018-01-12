#ifndef SOCKET_WIN_H
#define SOCKET_WIN_H

#include <net.h>

#include <winsock2.h>
#include <Windows.h>
#include <Ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <strsafe.h>
namespace pp {
    namespace net {
        int Socket::create(errors::Error &e)
        {
            m_fd = WSASocket(AF_INET, SOCK_STREAM, 0,
                NULL, 0, WSA_FLAG_OVERLAPPED);
            if (m_fd == INVALID_SOCKET) {
                return -1;
            }
            return m_fd;
        }




    }
}

#endif