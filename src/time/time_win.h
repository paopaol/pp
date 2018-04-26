#ifndef TIME_WIN_H
#define TIME_WIN_H

#include <Windows.h>
#include <WinSock2.h>
#include <stdint.h>

extern void gettimeofday(struct timeval *tp, void *unused);
#endif
