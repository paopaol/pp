#ifndef __SIGNAL_H_
#define __SIGNAL_H_

#include <cassert>
#include <cstddef>
#include <functional>
#include <io/io_event_loop.h>
#include <list>
#include <mutex>
#include <thread>
#include <type_traits>

template <typename Type, typename... Args>
static inline std::function<void(Args...)>
createFunctor(Type *instance, void (Type::*method)(Args... args)) {
  return [=](Args &&... args) { (instance->*method)(args...); };
}

template <typename func_type> class signal;
class object {
public:
  enum class ConnectionType { kAuto };
  explicit object(object *parent = nullptr)
      : parent_(parent), threadId_(std::this_thread::get_id()) {}

  std::thread::id threadId() { return threadId_; }

  template <typename SenderType, typename func_type, typename ReciverType,
            typename... Args>
  static inline bool connect(SenderType *sender, signal<func_type> &sig,
                             ReciverType *reciver,
                             void (ReciverType::*method)(Args... args),
                             ConnectionType type = ConnectionType::kAuto) {
    static_assert(std::is_base_of<object, SenderType>::value, "invalid type");
    static_assert(std::is_base_of<object, ReciverType>::value, "invalid type");
    assert((sender->threadId() == sig.threadId()) &&
           "sender and signal must be below the same thread!");
    auto _slot = createFunctor(reciver, method);
    sig.appendSlot(type, nullptr, _slot);
    return true;
  }

private:
  object *parent_;
  std::thread::id threadId_;
  pp::io::event_loop *loop_;
};

template <typename func_type> class slot {
public:
  using Functor = std::function<func_type>;
  slot(const Functor &functor) : functor_(functor) {}
  template <typename... Args> void run(Args... args) {
    if (functor_) {
      functor_(args...);
    }
  }

private:
  Functor functor_;
};

template <typename func_type> class signal {
  template <typename func_type> struct slot_entry {
  public:
    slot_entry(object::ConnectionType type, pp::io::event_loop *_loop,
               const slot<func_type> &_slot)
        : connectionType(type), loop(_loop), slot(slot) {}

    pp::io::event_loop *loop;
    object::ConnectionType connectionType;
    slot<func_type> slot;
  };

public:
  signal() : threadId_(std::this_thread::get_id()) {}
  std::thread::id threadId() { return threadId_; }

  void appendSlot(object::ConnectionType type, pp::io::event_loop *loop,
                  const slot<func_type> &_slot) {
    slot_entry<func_type> entry{type, loop, _slot};
    std::unique_lock<std::mutex> lock(mutex_);
    slots_.push_back(entry);
  }
  template <typename... Args> void emit(Args... args) {
    SlotList<func_type> tempSlots;
    {
      std::unique_lock<std::mutex> lock(mutex_);
      tempSlots = slots_;
    }
    for (auto &slotEntry : tempSlots) {
      if (slotEntry.connectionType == object::ConnectionType::kAuto) {
      }
      slotEntry.slot.run(args...);
    }
  }

private:
  std::thread::id threadId_;
  template <typename func_typr>
  using SlotList = std::list<slot_entry<func_type>>;
  SlotList<func_type> slots_;
  std::mutex mutex_;
};

#endif // __SIGNAL_H_
