
//
// Created by jz on 17-2-26.
//

#include <time.h>
#include <time.h>
#include <stdio.h>
#include <string.h>

#if defined( WIN32 )
#include <time_win.h>
#include <Windows.h>
#else
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>
#endif


#include <iostream>
#include <vector>

#include <_time.h>

namespace pp{
    namespace _time{
        using namespace std;
        const static string months[] = {
            "January",
            "February",
            "March",
            "April",
            "May",
            "June",
            "July",
            "August",
            "September",
            "October",
            "November",
            "December",
        };

        const static string days[] = {
            "Sunday",
            "Monday",
            "Tuesday",
            "Wednesday",
            "Thursday",
            "Friday",
            "Saturday",
        };

        static const Duration minDuration = (-1 << 63);
        static const Duration maxDuration = (1 << 63) - 1;



        Month::Month(int month) {
            m = month;
        }

        std::string Month::String() const {
            return months[m - 1];
        }

        Weekday::Weekday(int w) {
            wd = w;
        }

        std::string Weekday::String() const {
            return days[wd];
        }

        enum {
            secondsPerMinute = 60,
            secondsPerHour   = 60 * 60,
            secondsPerDay    = 24 * secondsPerHour,
            secondsPerWeek   = 7 * secondsPerDay,
            daysPer400Years  = 365*400 + 97,
            daysPer100Years  = 365*100 + 24,
            daysPer4Years    = 365*4 + 1
        };










        Time::Time() {
            struct  timeval     tv;

            gettimeofday(&tv, NULL);
            sec = tv.tv_sec;
            nsec = tv.tv_usec * 1000;
            // localtime(&tm, (time_t *)&sec);
#ifdef WIN32
            localtime_s(&tm, &sec );
#else
            localtime_r(&sec, &tm);
#endif
        }

        bool Time::After(Time u) {
            return this->sec > u.sec || (this->sec == u.sec && this->nsec > u.nsec);
        }

        bool Time::Before(Time u) {
            return this->sec < u.sec || (this->sec == u.sec && this->nsec < u.nsec);
        }

        bool Time::Equal(Time u) {
            return this->sec == u.sec && this->nsec == u.nsec;
        }

        Time Time::Add(Duration d) {
            sec += d / (int64_t)1e9;

            int32_t nano = nsec + (int32_t)(d % (int64_t)1e9);
            if (nano >= (int64_t)1e9){
                sec++;
                nano -= (int64_t)1e9;
            }else if (nano < 0){
                sec--;
                nano += (int64_t)1e9;
            }
            nsec = nano;

#ifdef WIN32
            localtime_s(&tm, &sec );
#else
            localtime_r(&sec, &tm);
#endif
            return *this;
        }

        Duration Time::Sub(Time &u)
        {
            Duration d = (Duration)(sec - u.sec) * _time::Second + (Duration)(nsec - u.nsec);

            if (u.Add(d).Equal(*this)){
                return d;
            }
            if (this->Before(u)){
                return minDuration;
            }
            return maxDuration;

        }


        int64_t Time::Unix() {
            return sec;
        }

        int64_t Time::UnixNano() {
            return (sec) * 1000 * 1000 * 1000 + (int64_t)nsec;
        }

        int Time::Year() {
            return tm.tm_year + 1900;
        }

        Month Time::_Month() {
            Month   m(tm.tm_mon + 1);

            return m;
        }

        int Time::Day() {
            return tm.tm_mday;
        }

        int Time::Hour() {
            return tm.tm_hour;
        }

        int Time::Minute() {
            return tm.tm_min;
        }

        int Time::Second() {
            return tm.tm_sec;
        }

        int Time::Nanosecond() {
            return nsec;
        }

        int Time::YearDay() {
            return tm.tm_yday + 1;
        }


        std::string Time::Format(std::string layout) {
            char    buf[128] = {0};

            strftime(buf, sizeof(buf), layout.c_str(), &tm);

            return string(buf);
        }

        string Time::String() {
            char    buf[128] = {0};
            char    str[1024] = {0};

            strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
            snprintf(str,  sizeof(str) - 1, "%s.%06d", buf, nsec);

            return string(str);
        }











        Time Now(){
            Time    now;

            return now;
        }

        // void Sleep(Duration d){
        //     // ::Sleep((DWORD)(d / Millisecond));
        //     sleep(d / Second);
        // }

        //string DurationString(Duration d){
        //    std::vector<char>   buf;

        //    buf.resize(32);
        //    int w = buf.size();

        //    uint64_t u = (uint64_t)d;
        //    bool neg = d < 0;

        //    if (neg){
        //        u = -u;
        //    }

        //    if (u < (uint64_t)_time::Second){
        //        int     prec = 0;

        //        w--;
        //        buf[w] = 's';
        //        w--;
        //        if (u == 0){
        //            return "0s";
        //        }else if (u < (uint64_t)_time::Microsecond){
        //            prec = 0;
        //            buf[w] = 'n';
        //        }else if (u < (uint64_t)_time::Millisecond){
        //            prec = 3;
        //            buf[w] = 0xB5;
        //            w--;
        //            buf[w] = 0xC2;
        //        }else {
        //            prec = 6;
        //            buf[w] = 'm';
        //        }
        //

        //    }else{

        //    }

        //}

        //Timer::Timer(Duration d) {
        //    Time                now;
        //    Time                after;
        //    struct itimerspec  timer;/* Interval for periodic timer */
        //    struct timespec   value;/* Initial expiration */

        //    after = now.Add(d);

        //    r.when = after.UnixNano();
        //    r.func = NULL;
        //    r.arg = NULL;
        //    r.period = 0;
        //    r.seq = 0;

        //    timerfd =  timerfd_create(CLOCK_REALTIME, 0);

        //    if (timerfd >= 0){
        //        timer.it_value.tv_sec = after.Unix();
        //        timer.it_value.tv_nsec = after.Nanosecond();

        //        timer.it_interval.tv_sec = 0;
        //        timer.it_interval.tv_nsec = 0;

        //        timerfd_settime(timerfd, TFD_TIMER_ABSTIME, &timer, NULL);
        //    }
        //}

        //uint64_t Timer::Fd() {
        //    return timerfd;
        //}


    }
}
