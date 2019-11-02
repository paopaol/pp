#include <system/sys_condition.h>

#include <Windows.h>

namespace pp {
namespace system {

class ConditionPrivate {
public:
  ConditionPrivate(Mutex &mutex) : mutex_(mutex) {
    InitializeConditionVariable(&cond_);
  }
  void wait() {
    SleepConditionVariableCS(&cond_, (PCRITICAL_SECTION)mutex_.getMutex(),
                             INFINITE);
  }

  void notify() { WakeConditionVariable(&cond_); }

  void notifyAll() { WakeAllConditionVariable(&cond_); }

private:
  Mutex &mutex_;
  CONDITION_VARIABLE cond_;
};

Condition::Condition(Mutex &mutex) : priv_(new ConditionPrivate(mutex)) {}

Condition::~Condition() { delete priv_; }

void Condition::wait() { priv_->wait(); }

void Condition::notify_one() { priv_->notify(); }

void Condition::notify_all() { priv_->notifyAll(); }

} // namespace system
} // namespace pp
