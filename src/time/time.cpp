
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

month::month(int month) { m = month; }

std::string month::string() const { return months[m - 1]; }

int month::number() { return m; }

weekday::weekday(int w) { wd = w; }

std::string weekday::string() const { return days[wd]; }

enum {
  secondsPerMinute = 60,
  secondsPerHour = 60 * secondsPerMinute,
  secondsPerDay = 24 * secondsPerHour,
  secondsPerWeek = 7 * secondsPerDay,
  daysPer400Years = 365 * 400 + 97,
  daysPer100Years = 365 * 100 + 24,
  daysPer4Years = 365 * 4 + 1
};

time::time() {
  struct timeval tv;

  gettimeofday(&tv, NULL);
  sec = tv.tv_sec;
  nsec = tv.tv_usec * 1000;
  // localtime(&tm, (time_t *)&sec);
#ifdef WIN32
  localtime_s(&tm, &sec);
#else
  localtime_r(&sec, &tm);
#endif
}

time::time(const time &t) {
  sec = t.sec;
  nsec = t.nsec;
  tm = t.tm;
}

time &time::operator=(const time &t) {
  sec = t.sec;
  nsec = t.nsec;
  tm = t.tm;
  return *this;
}

bool time::after(time u) {
  return this->sec > u.sec || (this->sec == u.sec && this->nsec > u.nsec);
}

bool time::before(time u) {
  return this->sec < u.sec || (this->sec == u.sec && this->nsec < u.nsec);
}

bool time::equal(time u) { return this->sec == u.sec && this->nsec == u.nsec; }

time time::add(Duration d) {
  sec += d / (int64_t)1e9;

  int32_t nano = nsec + (int32_t)(d % (int64_t)1e9);
  if (nano >= (int64_t)1e9) {
    sec++;
    nano -= (int64_t)1e9;
  } else if (nano < 0) {
    sec--;
    nano += (int64_t)1e9;
  }
  nsec = nano;

#ifdef WIN32
  localtime_s(&tm, &sec);
#else
  localtime_r(&sec, &tm);
#endif
  return *this;
}

Duration time::sub(time &u) {
  time uu = u;
  Duration d =
      (Duration)(sec - uu.sec) * _time::Second + (Duration)(nsec - uu.nsec);

  if (uu.add(d).equal(*this)) {
    return d;
  }
  if (this->before(uu)) {
    return minDuration;
  }
  return maxDuration;
}

int64_t time::unix() { return sec; }

int64_t time::unixnano() { return (sec)*1000 * 1000 * 1000 + (int64_t)nsec; }

int time::year() { return tm.tm_year + 1900; }

month time::month() {
  _time::month m(tm.tm_mon + 1);

  return m;
}

int time::mday() { return tm.tm_mday; }

int time::hour() { return tm.tm_hour; }

int time::minute() { return tm.tm_min; }

int time::second() { return tm.tm_sec; }

int time::nanosecond() { return nsec; }

int64_t time::millisecond() { return unixnano() / _time::Millisecond; }

int time::yearday() { return tm.tm_yday + 1; }

std::string time::format(std::string layout) {
  char buf[128] = {0};

  strftime(buf, sizeof(buf), layout.c_str(), &tm);

  return std::string(buf);
}

string time::string() const {
  char buf[128] = {0};
  char str[1024] = {0};

  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
  snprintf(str, sizeof(str) - 1, "%s.%06d", buf, nsec);

  return str;
}

time now() {
  time now;

  return now;
}

} // namespace _time
} // namespace pp
