#ifndef CSTDCALL_BLOCKING_QUEUE_H
#define CSTDCALL_BLOCKING_QUEUE_H

#include <cassert>
#include <list>
#include <system/sys_condition.h>

namespace pp {
namespace sync {

    template <typename T> class Chan {
    public:
        Chan(int size = 1)
            : maxSize_(size),
              closed_(false),
              mutex_(),
              not_empty_cond_(mutex_),
              not_full_cond_(mutex_)
        {
            assert(maxSize_ >= 0 && "the chan of size can't be < 0");
        }

        // if chan closed, write return false
        bool write(const T& x)
        {

            system::MutexLockGuard lock(mutex_);
            while (queue_.size() == maxSize_ && !closed_) {
                not_full_cond_.wait();
            }
            if (closed_) {
                return false;
            }
            queue_.push_back(x);
            not_empty_cond_.notify_one();

            return true;
        }

        // if chan closed, read return false
        bool read(T& front)
        {
            system::MutexLockGuard lock(mutex_);

            while (queue_.empty() && !closed_) {
                not_empty_cond_.wait();
            }
            if (closed_ && queue_.empty()) {
                return false;
            }

            front = queue_.front();
            queue_.pop_front();
            not_full_cond_.notify_one();
            return true;
        }

        void close()
        {
            system::MutexLockGuard lock(mutex_);
            closed_ = true;
            not_empty_cond_.notify_all();
            not_full_cond_.notify_all();
        }

        void reset()
        {
            system::MutexLockGuard lock(mutex_);
            closed_ = false;
            queue_.clear();
        }

        // size_t size()
        //{
        //    system::MutexLockGuard  lock(mutex_);
        //  return queue_.size();
        //}
        //
        // bool full()
        //{
        //    system::MutexLockGuard  lock(mutex_);
        //  return queue_.size() == maxSize_;
        //}

    private:
        Chan(const Chan&);
        const Chan& operator=(const Chan&);

    private:
        mutable system::Mutex mutex_;
        system::Condition     not_empty_cond_;
        system::Condition     not_full_cond_;
        int                   maxSize_;
        std::list<T>          queue_;
        bool                  closed_;
    };
}  // namespace sync
}  // namespace pp

#endif
