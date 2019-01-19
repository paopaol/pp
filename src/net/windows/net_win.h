#ifndef NET_WIN_H
#define NET_WIN_H

#include <errors/pp_error.h>
#include <windows/errors_windows.h>

#include <WinSock2.h>
#include <Windows.h>

#define make_win_socket_error_code(error, fd)                  \
    error = hht_make_error_code(                               \
        static_cast<errors::error>(errors::error::NET_ERROR)); \
    error.suffix_msg(errors::win_errstr(::WSAGetLastError())); 

#endif
