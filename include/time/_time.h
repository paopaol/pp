//
// Created by jz on 17-2-26.
//

#ifndef PP_time_H
#define PP_time_H

#include <functional>
#include <memory>
#include <queue>
#include <stdint.h>
#include <string>
#include <sys/types.h>

namespace pp {
namespace io {
    class event_loop;
}
}  // namespace pp

namespace pp {
namespace _time {
    typedef int64_t Duration;

    const Duration Nanosecond  = 1;
    const Duration Microsecond = 1000 * Nanosecond;
    const Duration Millisecond = 1000 * Microsecond;
    const Duration Second      = 1000 * Millisecond;
    const Duration Minute      = 60 * Second;
    const Duration Hour        = 60 * Minute;

    class month {
    public:
        month(int month);

        std::string string() const;
		int number();

    private:
        int m;
    };

    class weekday {
    public:
        weekday(int w);

        std::string string() const;

    private:
        int wd;
    };

    const month January   = 1;
    const month February  = 2;
    const month March     = 3;
    const month April     = 4;
    const month May       = 5;
    const month June      = 6;
    const month July      = 7;
    const month August    = 8;
    const month September = 9;
    const month October   = 10;
    const month November  = 11;
    const month December  = 12;

    const weekday Sunday    = 0;
    const weekday Monday    = 1;
    const weekday Tuesday   = 2;
    const weekday Wednesday = 3;
    const weekday Thursday  = 4;
    const weekday Friday    = 5;
    const weekday Saturday  = 6;

    class time {
    public:
        time();
        time(const time& t);

        bool after(time u);

        bool before(time u);

        bool equal(time u);

        time     add(Duration d);
        Duration sub(time& u);

        int64_t unix();
        int64_t unixnano();
        int     year();
        month   month();
        int     mday();
        int     hour();
        int     minute();
        int     second();
        int     nanosecond();
        int64_t millisecond();

        int yearday();

        std::string format(std::string layout = "%Y-%m-%d %H:%M:%S");
        std::string string() const;

        time& operator=(const time& t);

    private:
        int64_t sec;
        int32_t nsec;

        struct tm tm;
        //        Location        *loc;
    };

    time now();

    class timer;
    typedef std::function<void()>  timer_handler;
    typedef std::shared_ptr<timer> timer_ref;
    class timer : public std::enable_shared_from_this<timer> {
    public:
        enum timer_behavior { interval, oneshot };

        void cancel();
		bool canceled();
        int64_t        future();
        timer_behavior behavior();
        ~timer(){};
        friend struct timer_cmper;

        friend timer_ref new_timer(timer::timer_behavior be, Duration when,
                                   const timer_handler& func);

    private:
        explicit timer(timer_behavior be, int64_t when,
                       const timer_handler& func, io::event_loop* loop);

        friend class timer_queue;
        // friend class io::event_loop;

        void run_timeout();

        timer(const timer& other);
        timer& operator=(const timer& other);

        timer_handler func_;
        // when timer handler need run, ms
        Duration when_;
        int64_t  future_;
        bool     canceled_;
        // behavior of timer, interval/oneshot
        timer_behavior  how_;
        io::event_loop* loop_;
    };

    struct timer_cmper {
        bool operator()(timer_ref a, timer_ref b)
        {
            return a->future_ > b->future_;
        }
    };

    // one timer_queue per thread
    // we will use thread local data create it
    class timer_queue {
    public:
        //! Default constructor
        timer_queue();

        //! Destructor
        ~timer_queue(){};

        // if timer already exit, update it
        void      push(const timer_ref& timer);
        void      pop();
        timer_ref top();
        int64_t   handle_timeout_timer();
        int       size();

    private:
        timer_queue(const timer_queue& other);
        timer_queue& operator=(const timer_queue& other);

        std::priority_queue<timer_ref, std::vector<timer_ref>, timer_cmper>
            timer_queue_;
    };
    typedef std::shared_ptr<timer_queue> timer_queue_ref;

    timer_ref new_timer(timer::timer_behavior be, int64_t when,
                        const timer_handler& func);

}  // namespace _time

}  // namespace pp

#endif  // PP_time_H
