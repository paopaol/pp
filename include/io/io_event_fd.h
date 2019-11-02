#ifndef EVENT_FD_H
#define EVENT_FD_H

#include <bytes/buffer.h>
#include <errors/hht_error.h>
#include <hht.h>

#include <functional>
#include <vector>

#include <hht.h>
#include <memory>

namespace pp {
namespace io {

class event_loop;
class event_fd {
public:
  static const int EV_NONE = 0;
  static const int EV_READ = 1;
  static const int EV_WRITE = EV_READ << 2;
  static const int EV_CLOSE = EV_WRITE << 1;
  static const int EV_ERROR = EV_CLOSE << 1;

  typedef std::function<void(errors::error_code &error)> event_handler;

  event_fd(event_loop *loop, int fd);
  ~event_fd() {}
  void set_write_handler(const event_handler &handler);
  void data_recved(const event_handler &handler);
  void set_error_handler(const event_handler &handler);
  void closed(const event_handler &handler);

  void handle_event();
  void handle_event_with_guard();
  void set_active(int events);

  void enable_read(errors::error_code &error);
  void enable_write(errors::error_code &error);
  int enabled_event();

  inline int fd() { return fd_; };
  void tie(const std::shared_ptr<void> &obj) {
    tie_ = obj;
    tied_ = true;
  }

  event_loop *its_loop();

  void update_event(errors::error_code &error);
  void remove_event(errors::error_code &error);

  event_loop *event_loop_;
  int fd_;
  int enabled_event_;
  int active_event_;
  event_handler handler_write_;
  event_handler handler_read_;
  event_handler handler_close_;
  event_handler handler_error_;
  std::weak_ptr<void> tie_;
  bool tied_;

private:
  DISABLE_COPY_CONSTRCT(event_fd);
};

typedef std::shared_ptr<event_fd> event_fd_ref;
} // namespace io
} // namespace pp

#endif // EVENT_FD_H
