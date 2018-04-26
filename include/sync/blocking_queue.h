#ifndef CSTDCALL_BLOCKING_QUEUE_H
#define CSTDCALL_BLOCKING_QUEUE_H

#include <deque>

#include "mutex.h"
#include "condition.h"

namespace cstdcall{
	namespace sync{

		template<typename T>
		class BlockingQueue{
		public:
			BlockingQueue():
			  m_mutex(),
				  m_notEmptyCond(m_mutex),
				  m_queue()
			  {
				  m_stop = false;
			  }

			  bool put(const T& x)
			  {
				  MutexLockGuard  lock(m_mutex);
				  if (m_stop){
					  return false;
				  }
				  m_queue.push_back(x);
				  m_notEmptyCond.notify();
				  return true;
			  }

			  bool get(T &front)
			  {
				  MutexLockGuard  lock(m_mutex);
				  while(m_queue.empty() && !m_stop){
					  m_notEmptyCond.wait();
				  }
				  if (m_stop){
					  return false;
				  }

				  front = m_queue.front();
				  m_queue.pop_front();
				  return true;
			  }

			  void stop()
			  {
				  MutexLockGuard  lock(m_mutex);
				  m_stop = true;
				  m_notEmptyCond.notifyAll();
			  }

			  size_t size()
			  {
				  MutexLockGuard  lock(m_mutex);
				  return m_queue.size();
			  }


		private:
			BlockingQueue(const BlockingQueue &);
			const BlockingQueue& operator = (const BlockingQueue &);

		private:
			//mutex cond 声明顺序很重要，不能颠倒
			mutable Mutex		    m_mutex;
			Condition               m_notEmptyCond;
			std::deque<T>           m_queue;
			bool                    m_stop;
		};
	}
}


#endif