//
// Created by jz on 17-2-26.
//

#ifndef PP_TIME_H
#define PP_TIME_H

#include <queue>
#include <functional>
#include <stdint.h>
#include <string>
#include <sys/types.h>
#include <stdint.h>
#include <memory>

namespace pp{
	namespace io {
		class event_loop;
	}
}


namespace pp {
namespace _time {
    typedef int64_t Duration;

    const Duration Nanosecond  = 1;
     const Duration Microsecond = 1000 * Nanosecond;
	const Duration Millisecond = 1000 * Microsecond;
    const Duration Second      = 1000 * Millisecond;
    const Duration Minute      = 60 * Second;
    const Duration Hour        = 60 * Minute;

    class Month {
    public:
        Month(int month);

        std::string String() const;

    private:
        int m;
    };

    class Weekday {
    public:
        Weekday(int w);

        std::string String() const;

    private:
        int wd;
    };

    const Month January   = 1;
    const Month February  = 2;
    const Month March     = 3;
    const Month April     = 4;
    const Month May       = 5;
    const Month June      = 6;
    const Month July      = 7;
    const Month August    = 8;
    const Month September = 9;
    const Month October   = 10;
    const Month November  = 11;
    const Month December  = 12;

    const Weekday Sunday    = 0;
    const Weekday Monday    = 1;
    const Weekday Tuesday   = 2;
    const Weekday Wednesday = 3;
    const Weekday Thursday  = 4;
    const Weekday Friday    = 5;
    const Weekday Saturday  = 6;

    class Time {
    public:
        Time();
		Time(const Time &t);

        bool After(Time u);

        bool Before(Time u);

        bool Equal(Time u);

        Time     Add(Duration d);
        Duration Sub(Time& u);

        int64_t Unix();
        int64_t UnixNano();
        int     Year();
        Month   _Month();
        int     Day();
        int     Hour();
        int     Minute();
        int     Second();
        int     Nanosecond();
		int64_t Millisecond();

        int YearDay();

        std::string Format(std::string layout);
        std::string String() const;

		Time &operator=(const Time &t);

    private:
        int64_t sec;
        int32_t nsec;

        struct tm tm;
        //        Location        *loc;
    };

    Time Now();

   
	class timer;
    typedef std::function<void()> timer_handler;
	typedef std::shared_ptr<timer> timer_ref;
    class timer : public std::enable_shared_from_this<timer>{
	public:
		enum timer_behavior { interval, oneshot };

        void cancel();
		int64_t future();
		timer_behavior behavior();
		~timer() {};
		friend struct timer_cmper;
	
		friend timer_ref new_timer(timer::timer_behavior be, Duration when,
			const timer_handler& func);
	private:

		 explicit timer(timer_behavior be, int64_t when,
			const timer_handler& func, io::event_loop* loop);


        friend class timer_queue;
		//friend class io::event_loop;
      
        void run_timeout();


        timer(const timer& other);
        timer& operator=(const timer& other);
       

        timer_handler func_;
        // when timer handler need run, ms
		Duration when_;
		int64_t future_;
        // behavior of timer, interval/oneshot
		timer_behavior         how_;
		io::event_loop* loop_;
    };

	struct timer_cmper {
		bool operator()(timer_ref a, timer_ref b) {
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
		 ~timer_queue() {};

        // if timer already exit, update it
        void      push(const timer_ref& timer);
        void      pop();
        timer_ref top();
		int64_t handle_timeout_timer();
		int64_t next_timeout();
		int size();

        
	private:
        timer_queue(const timer_queue& other);
        timer_queue& operator=(const timer_queue& other);

        std::priority_queue<timer_ref, std::vector<timer_ref>, timer_cmper> timer_queue_;
    };
	typedef std::shared_ptr<timer_queue> timer_queue_ref;

    timer_ref new_timer(timer::timer_behavior be, int64_t when,
                        const timer_handler& func);

}  // namespace _time

}  // namespace pp

#endif  // PP_TIME_H
