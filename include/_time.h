//
// Created by jz on 17-2-26.
//

#ifndef CSTDCALL_TIME_H
#define CSTDCALL_TIME_H

#include <sys/types.h>
#include <string>
#include <stdint.h>
#include <functional>



namespace pp{
    namespace _time{
        typedef int64_t Duration;

        const Duration    Nanosecond           = 1;
        const Duration    Microsecond          = 1000 * Nanosecond;
        const Duration    Millisecond          = 1000 * Microsecond;
        const Duration    Second               = 1000 * Millisecond;
        const Duration    Minute               = 60 * Second;
        const Duration    Hour                 = 60 * Minute;



        class Month {
        public:
            Month(int month);

            std::string String() const;

        private:
            int     m;
        };


        class Weekday {
        public:
            Weekday(int w);

            std::string String() const ;

        private:
            int         wd;
        };


        const Month     January = 1;
        const Month     February = 2;
        const Month     March = 3;
        const Month     April = 4;
        const Month     May = 5;
        const Month     June = 6;
        const Month     July = 7;
        const Month     August = 8;
        const Month     September = 9;
        const Month     October = 10;
        const Month     November = 11;
        const Month     December = 12;

        const Weekday   Sunday = 0;
        const Weekday   Monday = 1;
        const Weekday   Tuesday = 2;
        const Weekday   Wednesday = 3;
        const Weekday   Thursday = 4;
        const Weekday   Friday = 5;
        const Weekday   Saturday = 6;








        class Time {
        public:
            Time();

            bool After(Time u);

            bool Before(Time u);

            bool Equal(Time u);

            Time Add(Duration d);
            Duration Sub(Time &u);

            int64_t Unix();
            int64_t UnixNano();
            int Year();
            Month _Month();
            int Day();
            int Hour();
            int Minute();
            int Second();
            int Nanosecond();

            int YearDay();

            std::string Format(std::string layout);
            std::string String();




        private:
            int64_t         sec;
            int32_t         nsec;

            struct tm       tm;
            //        Location        *loc;
        };

        Time Now();
        void Sleep(Duration d);

	//typedef void (*TimerFunc)(void *, bool);


		//struct runtimeTimer{
		//		runtimeTimer();

		//    int64_t         when;
		//    int64_t         period;
		//		TimerFunc		func;
		//		void			*arg;
		//    HANDLE          timerfd;
		//};



		//	class Timer;
		//	typedef std::tr1::shared_ptr<Timer>	TimerRef;

		//class Timer{
		//	public:
		//		friend class TimerQueue;

		//		bool Stop(error_t &err);
		//private:
		//    runtimeTimer				r;
		//		std::tr1::shared_ptr<Timer>	thisP;
		//};

		//	TimerRef NewTimer(Duration d, TimerFunc f, void *arg, error_t &err);

    }

}




#endif //CPP_TIME_H
