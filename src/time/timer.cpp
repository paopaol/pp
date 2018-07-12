#include <io/io_event_loop.h>
#include <time/_time.h>

#ifdef WIN32
#include <windows/sys_win_thread_local_storage.h>
#endif

namespace pp {
namespace _time {
    timer::timer(timer_behavior be, int64_t when, const timer_handler& func,
                 io::event_loop* loop)
        : loop_(loop), how_(be), when_(when), func_(func)
    {
        assert(loop_
               && "timer create failed, not exits event loop in current "
                  "thread");
        auto now = Now();
        future_  = now.Add(when).Millisecond();
        ;
    }

    void timer::cancel()
    {
        assert(loop_->in_created_thread()
               && "timer must cancel in it's construct thread");
        func_ = nullptr;
    }

    void timer::run_timeout()
    {
        assert(loop_->in_created_thread()
               && "timer must use in construct thread");
        if (func_) {
            func_();
        }
        if (behavior() != oneshot) {
            future_ += when_ / Millisecond;
        }
    }

    int64_t timer::future()
    {
        assert(loop_->in_created_thread()
               && "timer must use in construct thread");
        return future_;
    }

    timer::timer_behavior timer::behavior()
    {
        assert(loop_->in_created_thread()
               && "timer must use in construct thread");
        return how_;
    }

    timer_ref new_timer(timer::timer_behavior be, Duration when,
                        const timer_handler& func)
    {
        io::event_loop* loop =
            static_cast<io::event_loop*>(loop_curren_thread_loop());
        timer_ref t(new timer(be, when, func, loop));
        loop->get_timer_queue()->push(t);
        loop->wakeup();

        return t;
    }

}  // namespace _time
}  // namespace pp
