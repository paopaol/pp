
//
// Created by jz on 17-2-26.
//
#include <io/io_event_loop.h>
#include <time/_time.h>

#include <stdio.h>
#include <string.h>
#include <time.h>

#if defined(WIN32)
#include "time_win.h"
#if _MSC_VER
#define snprintf _snprintf
#endif
#else
#include <fcntl.h>
#include <sys/time.h>
#include <unistd.h>
#endif

#include <iostream>
#include <vector>

namespace pp {
namespace _time {
    using namespace std;
    const static string months[] = {
        "January", "February", "March",     "April",   "May",      "June",
        "July",    "August",   "September", "October", "November", "December",
    };

    const static string days[] = {
        "Sunday",   "Monday", "Tuesday",  "Wednesday",
        "Thursday", "Friday", "Saturday",
    };

    static const Duration minDuration = (-1 << 63);
    static const Duration maxDuration = (1 << 63) - 1;

    Month::Month(int month)
    {
        m = month;
    }

    std::string Month::String() const
    {
        return months[m - 1];
    }

    Weekday::Weekday(int w)
    {
        wd = w;
    }

    std::string Weekday::String() const
    {
        return days[wd];
    }

    enum {
        secondsPerMinute = 60,
        secondsPerHour   = 60 * 60,
        secondsPerDay    = 24 * secondsPerHour,
        secondsPerWeek   = 7 * secondsPerDay,
        daysPer400Years  = 365 * 400 + 97,
        daysPer100Years  = 365 * 100 + 24,
        daysPer4Years    = 365 * 4 + 1
    };

    Time::Time()
    {
        struct timeval tv;

        gettimeofday(&tv, NULL);
        sec  = tv.tv_sec;
        nsec = tv.tv_usec * 1000;
        // localtime(&tm, (time_t *)&sec);
#ifdef WIN32
        localtime_s(&tm, &sec);
#else
        localtime_r(&sec, &tm);
#endif
    }

    Time::Time(const Time& t)
    {
        sec  = t.sec;
        nsec = t.nsec;
        tm   = t.tm;
    }

    Time& Time::operator=(const Time& t)
    {
        sec  = t.sec;
        nsec = t.nsec;
        tm   = t.tm;
        return *this;
    }

    bool Time::After(Time u)
    {
        return this->sec > u.sec || (this->sec == u.sec && this->nsec > u.nsec);
    }

    bool Time::Before(Time u)
    {
        return this->sec < u.sec || (this->sec == u.sec && this->nsec < u.nsec);
    }

    bool Time::Equal(Time u)
    {
        return this->sec == u.sec && this->nsec == u.nsec;
    }

    Time Time::Add(Duration d)
    {
        sec += d / ( int64_t )1e9;

        int32_t nano = nsec + (int32_t)(d % ( int64_t )1e9);
        if (nano >= ( int64_t )1e9) {
            sec++;
            nano -= ( int64_t )1e9;
        }
        else if (nano < 0) {
            sec--;
            nano += ( int64_t )1e9;
        }
        nsec = nano;

#ifdef WIN32
        localtime_s(&tm, &sec);
#else
        localtime_r(&sec, &tm);
#endif
        return *this;
    }

    Duration Time::Sub(Time& u)
    {
        Time     uu = u;
        Duration d  = (Duration)(sec - uu.sec) * _time::Second
                     + (Duration)(nsec - uu.nsec);

        if (uu.Add(d).Equal(*this)) {
            return d;
        }
        if (this->Before(uu)) {
            return minDuration;
        }
        return maxDuration;
    }

    int64_t Time::Unix()
    {
        return sec;
    }

    int64_t Time::UnixNano()
    {
        return ( sec )*1000 * 1000 * 1000 + ( int64_t )nsec;
    }

    int Time::Year()
    {
        return tm.tm_year + 1900;
    }

    Month Time::_Month()
    {
        Month m(tm.tm_mon + 1);

        return m;
    }

    int Time::Day()
    {
        return tm.tm_mday;
    }

    int Time::Hour()
    {
        return tm.tm_hour;
    }

    int Time::Minute()
    {
        return tm.tm_min;
    }

    int Time::Second()
    {
        return tm.tm_sec;
    }

    int Time::Nanosecond()
    {
        return nsec;
    }

    int64_t Time::Millisecond()
    {
        return UnixNano() / _time::Millisecond;
    }

    int Time::YearDay()
    {
        return tm.tm_yday + 1;
    }

    std::string Time::Format(std::string layout)
    {
        char buf[128] = { 0 };

        strftime(buf, sizeof(buf), layout.c_str(), &tm);

        return string(buf);
    }

    string Time::String() const
    {
        char buf[128]  = { 0 };
        char str[1024] = { 0 };

        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
        snprintf(str, sizeof(str) - 1, "%s.%06d", buf, nsec);

        return str;
    }

    Time Now()
    {
        Time now;

        return now;
    }

}  // namespace _time
}  // namespace pp
