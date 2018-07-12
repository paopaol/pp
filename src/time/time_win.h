#ifndef TIME_WIN_H
#define TIME_WIN_H

#include <WinSock2.h>
#include <Windows.h>
#include <stdint.h>

extern void gettimeofday(struct timeval* tp, void* unused);
#endif
