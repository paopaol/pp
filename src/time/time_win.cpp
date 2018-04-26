#include "time_win.h"

void gettimeofday(struct timeval *tp, void * unused)
{
    uint64_t  intervals;
    FILETIME  ft;

    GetSystemTimeAsFileTime(&ft);

    /*
    * A file time is a 64-bit value that represents the number
    * of 100-nanosecond intervals that have elapsed since
    * January 1, 1601 12:00 A.M. UTC.
    *
    * Between January 1, 1970 (Epoch) and January 1, 1601 there were
    * 134744 days,
    * 11644473600 seconds or
    * 11644473600,000,000,0 100-nanosecond intervals.
    *
    * See also MSKB Q167296.
    */

    intervals = ((uint64_t) ft.dwHighDateTime << 32) | ft.dwLowDateTime;
    intervals -= 116444736000000000;

    tp->tv_sec = (long) (intervals / 10000000);
    tp->tv_usec = (long) ((intervals % 10000000) / 10);

}
