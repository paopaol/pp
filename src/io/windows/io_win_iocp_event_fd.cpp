
#ifdef WIN32
#include "windows/io_win_iocp_event_fd.h"
#endif

#include <io/io_event_loop.h>

#include <memory>
#include <net/net.h>

using namespace std;

namespace pp {
namespace io {

iocp_event_fd::iocp_event_fd(event_loop *loop, int fd) : event_fd(loop, fd) {}

void iocp_event_fd::enable_accpet(const accpet_done_handler &done_handler,
                                  errors::error_code &error) {
  enabled_event_ |= iocp_event_fd::EV_ACCPET;
  handle_accpet_done_ = done_handler;
  event_loop_->update_event_fd(this, error);
}

void iocp_event_fd::enable_connect(const connect_done_handler &done_handler,
                                   errors::error_code &error) {
  enabled_event_ |= iocp_event_fd::EV_CONNECT;
  handle_connect_done_ = done_handler;
  event_loop_->update_event_fd(this, error);
}

int iocp_event_fd::handle_zero_done() {
  set_active(iocp_event_fd::EV_CLOSE);
  event_fd::handle_event();
  return 0;
}

int iocp_event_fd::handle_read_done() {
  assert(active_pending_req_ != nullptr);

  if (active_pending_req_->io_size == 0) {
    handle_zero_done();
    return 0;
  }
  set_active(iocp_event_fd::EV_READ);
  event_fd::handle_event();
  return 0;
}

int iocp_event_fd::handle_accept_done() {
  set_active(iocp_event_fd::EV_ACCPET);
  errors::error_code error;

  if (handle_accpet_done_) {
    handle_accpet_done_(error);
  }
  return 0;
}

int iocp_event_fd::handle_connnect_done() {
  set_active(iocp_event_fd::EV_CONNECT);
  errors::error_code error;
  if (handle_connect_done_) {
    handle_connect_done_(error);
  }
  return 0;
}

int iocp_event_fd::handle_write_done() {
  errors::error_code error;

  assert(active_pending_req_ != nullptr);
  if (active_pending_req_->io_size == 0) {
    handle_zero_done();
    return 0;
  }
  set_active(iocp_event_fd::EV_WRITE);
  event_fd::handle_event();
  return 0;
}

void iocp_event_fd::handle_event_with_guard() {
  if (active_pending_req_->IoOpt & iocp_event_fd::EV_READ) {
    handle_read_done();
  } else if (active_pending_req_->IoOpt & iocp_event_fd::EV_ACCPET) {
    handle_accept_done();
  } else if (active_pending_req_->IoOpt & iocp_event_fd::EV_WRITE) {
    handle_write_done();
  } else if (active_pending_req_->IoOpt & iocp_event_fd::EV_CONNECT) {
    handle_connnect_done();
  }
  active_event_ = event_fd::EV_NONE;
}

void iocp_event_fd::handle_event() {
  std::shared_ptr<void> guard;

  if (tied_) {
    guard = tie_.lock();
    if (guard) {
      handle_event_with_guard();
    }
  } else {
    handle_event_with_guard();
  }
}

iocp_event_fd::io_request_ref iocp_event_fd::create_io_request(int ev) {
  io_request_ref request = std::make_shared<struct io_request_t>();

  request->io_fd = fd();
  request->IoOpt |= ev;
  return request;
}

iocp_event_fd::io_request_ref iocp_event_fd::remove_active_request() {
  io_request_ref active;
  active.swap(active_pending_req_);
  return active;
}
void iocp_event_fd::set_active_pending(io_request_t *active) {
  auto find = io_request_list_.find(active);
  assert(find != io_request_list_.end() && "not found pending io request, bug");
  active_pending_req_ = find->second;
  io_request_list_.erase(find);
}

int iocp_event_fd::pending_request_size() {
  return static_cast<int>(io_request_list_.size());
}

void iocp_event_fd::queued_pending_request(const io_request_ref &request) {
  io_request_list_[request.get()] = request;
}

} // namespace io
} // namespace pp
