#ifndef CSTDCALL_BLOCKING_QUEUE_H
#define CSTDCALL_BLOCKING_QUEUE_H

#include <cassert>
#include <condition_variable>
#include <list>

namespace pp {
namespace sync {

template <typename T> class chan {
public:
  chan(int size = 1) : maxSize_(size), closed_(false) {
    assert(maxSize_ >= 0 && "the chan of size can't be < 0");
  }

  // if chan closed, write return false
  bool write(const T &x) {
    std::unique_lock<std::mutex> lock(mutex_);
    while (queue_.size() == maxSize_ && !closed_) {
      not_full_cond_.wait(lock);
    }
    if (closed_) {
      return false;
    }
    queue_.push_back(x);
    not_empty_cond_.notify_one();

    return true;
  }

  // if chan closed, read return false
  bool read(T &front) {
    std::unique_lock<std::mutex> lock(mutex_);

    while (queue_.empty() && !closed_) {
      not_empty_cond_.wait(lock);
    }
    if (closed_ && queue_.empty()) {
      return false;
    }

    front = queue_.front();
    queue_.pop_front();
    not_full_cond_.notify_one();
    return true;
  }

  void close() {
    std::unique_lock<std::mutex> lock(mutex_);
    closed_ = true;
    not_empty_cond_.notify_all();
    not_full_cond_.notify_all();
  }

  void reset() {
    std::unique_lock<std::mutex> lock(mutex_);
    closed_ = false;
    queue_.clear();
  }

  size_t size() {
    std::unique_lock<std::mutex> lock(mutex_);
    return queue_.size();
  }
  //
  // bool full()
  //{
  //    system::MutexLockGuard  lock(mutex_);
  //  return queue_.size() == maxSize_;
  //}

private:
  chan(const chan &);
  const chan &operator=(const chan &);

private:
  mutable std::mutex mutex_;
  std::condition_variable not_empty_cond_;
  std::condition_variable not_full_cond_;
  int maxSize_;
  std::list<T> queue_;
  bool closed_;
};
} // namespace sync
} // namespace pp

#endif
