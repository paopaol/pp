#include <io/io_event_loop.h>
#include <system/sys_thread_local.h>
#include <time/_time.h>

#include <assert.h>

namespace pp {
namespace _time {
timer::timer(timer_behavior be, int64_t when, const timer_handler &func,
             io::event_loop *loop)
    : loop_(loop), how_(be), when_(when), func_(func), canceled_(false) {
  assert(loop_ && "timer create failed, not exits event loop in current "
                  "thread");
  assert(when > 0 && "timer period can't be < 0");
  auto now = _time::now();
  future_ = now.add(when).millisecond();
}

void timer::cancel() {
  assert(loop_->in_created_thread() &&
         "timer must cancel in it's construct thread");
  // lazy cancel
  func_ = nullptr;
  canceled_ = true;
}

bool timer::canceled() { return canceled_; }

void timer::run_timeout() {
  assert(loop_->in_created_thread() && "timer must use in construct thread");
  if (func_) {
    func_();
  }
  if (behavior() != oneshot) {
    future_ += when_ / Millisecond;
  }
}

int64_t timer::future() {
  assert(loop_->in_created_thread() && "timer must use in construct thread");
  return future_;
}

timer::timer_behavior timer::behavior() {
  assert(loop_->in_created_thread() && "timer must use in construct thread");
  return how_;
}

timer_ref new_timer(timer::timer_behavior be, Duration when,
                    const timer_handler &func) {
  io::event_loop *loop =
      static_cast<io::event_loop *>(system::current_thread_loop());
  assert(loop && "current thread does not exist event loop");
  timer_ref t(new timer(be, when, func, loop));
  // loop->get_timer_queue()->push(t);
  loop->insert_timer(t);
  loop->wakeup();

  return t;
}

} // namespace _time
} // namespace pp
