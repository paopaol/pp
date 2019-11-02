#ifndef HHT_CONDITION_H
#define HHT_CONDITION_H

#include <system/sys_mutex.h>

namespace pp {
namespace system {
class ConditionPrivate;
class Condition {
public:
  explicit Condition(Mutex &mutex);
  ~Condition();
  void wait();
  void notify_one();
  void notify_all();

private:
  Condition(const Condition &);
  const Condition &operator=(const Condition &);

  ConditionPrivate *priv_;
};
} // namespace system
} // namespace pp

#endif