#include <io/io_event_loop.h>
#include <time/_time.h>

namespace pp {
namespace _time {
    timer_queue::timer_queue() {}

    void timer_queue::push(const timer_ref& timer)
    {
        timer_queue_.push(timer);
    }

    void timer_queue::pop()
    {
        timer_queue_.pop();
    }

    timer_ref timer_queue::top()
    {
        return timer_queue_.top();
    }

    int64_t timer_queue::next_timeout()
    {
        auto now = Now().Millisecond();
        if (size() == 0) {
            return -1;
        }
        timer_ref current = top();
        assert(current);
        int64_t future = current->future();
        int     period = future - now;
        if (period > 0) {
            return period;
        }
        return 0;
    }

    int64_t timer_queue::handle_timeout_timer()
    {
        auto now = Now().Millisecond();

        while (1) {
            if (size() == 0) {
                return -1;
            }
            timer_ref current = top();
            assert(current);
            int64_t future = current->future();
            int     period = future - now;
            if (period > 0) {
                return period;
            }
            current->run_timeout();
            pop();
            if (current->behavior() == timer::oneshot) {
                continue;
            }
            push(current);
        }
    }

    int timer_queue::size()
    {
        return timer_queue_.size();
    }

}  // namespace _time
}  // namespace pp
