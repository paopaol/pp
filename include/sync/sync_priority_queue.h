#ifndef SYNC_PRORITY_QUEUE_H
#define SYNC_PRORITY_QUEUE_H

#include <system/sys_condition.h>
#include <queue>

namespace pp
{
namespace sync
{

template <typename T, typename CMP_TYPE>
class PriorityQueue
{
  public:
    PriorityQueue(int maxSize)
        : m_max(maxSize), m_full(m_mutex), m_empty(m_mutex), m_close(false)
    {
    }

    int size()
    {
        system::MutexLockGuard lock(m_mutex);
        return m_queue.size();
    }
    bool poptop(T &t)
    {
        system::MutexLockGuard lock(m_mutex);
        while (!m_close && m_queue.empty())
        {
            m_empty.wait();
        }
        if (m_close && m_queue.empty())
        {
            return false;
        }
        t = m_queue.top();
        m_queue.pop();
        m_full.notify_one();
        return true;
    }
    void close()
    {
        system::MutexLockGuard lock(m_mutex);
        m_close = true;
        m_empty.notify_all();
        m_full.notify_all();
    }

    bool push(T t)
    {
        system::MutexLockGuard lock(m_mutex);
        while (!m_close && m_queue.size() == m_max)
        {
            m_full.wait();
        }
        if (m_close)
        {
            return false;
        }
        m_queue.push(t);
        m_empty.notify_one();
        return true;
    }

  private:
    system::Mutex m_mutex;
    system::Condition m_empty;
    system::Condition m_full;
    int m_max;
    bool m_close;
    std::priority_queue<T, std::vector<T>, CMP_TYPE> m_queue;
};

} // namespace sync
} // namespace pp

#endif
