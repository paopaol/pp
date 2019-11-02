#ifndef EVENT_LOOP_H
#define EVENT_LOOP_H

#include <hht.h>

#include <system/sys_mutex.h>
#include <system/sys_thread.h>
#include <time/_time.h>
#include <vector>

#ifdef WIN32
//#include <windows/io_win_overlapped_pipe.h>
#endif
#include <errors/hht_error.h>

namespace pp {
namespace _time {
class _time::timer;
}

namespace io {
typedef std::function<void()> Functor;

class event_fd;
class event_poller;
typedef std::vector<event_fd *> event_fd_list;
class event_loop {
public:
  event_loop();
  ~event_loop();
  void exec();
  void wakeup();
  void quit();
  void run_in_loop(const Functor &func);
  bool in_created_thread();
  static event_loop *loop_of_current_thread();

  // internal functions,maybe private
  void update_event_fd(event_fd *event, errors::error_code &error);
  void remove_event_fd(event_fd *event, errors::error_code &error);
  void insert_timer(const _time::timer_ref &timer);
  static event_loop *current_thread_loop();

private:
  // we will call _time::timer private functions
  // friend class _time::timer;

  void move_to_loop_thread(const Functor &func);

  DISABLE_COPY_CONSTRCT(event_loop);

  system::thread_id tid_;
  bool execing_;
  bool exit_;
  std::shared_ptr<event_poller> event_poller_;
  errors::error_code error;
  event_fd_list active_ev_fd_list_;
  _time::timer_queue_ref timer_queue_;
  std::vector<Functor> func_list_;
  system::Mutex func_list_mutex_;
};
} // namespace io
} // namespace pp

#endif // EVENT_LOOP_H
