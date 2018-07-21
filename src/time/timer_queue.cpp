#include <io/io_event_loop.h>
#include <time/_time.h>
#include <assert.h>
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

    int64_t timer_queue::handle_timeout_timer()
    {
        auto now = _time::now().millisecond();

        while (1) {
            if (size() == 0) {
                return -1;
            }
            timer_ref current = top();
            assert(current);

            if (current->canceled()) {
                pop();
                continue;
            }
            int64_t future = current->future();
            int64_t period = future - now;
            if (period > 0) {
                return period;
            }
            pop();
            current->run_timeout();
            if (current->behavior() == timer::oneshot) {
                continue;
            }
            // if the timer is a interval timer, so we need
            // push the popped timer into timerqueue
            push(current);
        }
    }

    int timer_queue::size()
    {
        return timer_queue_.size();
    }

}  // namespace _time
}  // namespace pp
