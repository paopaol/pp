#ifndef CSTDCALL_CONDITION_H
#define CSTDCALL_CONDITION_H 

#include "mutex.h"
#include "cstdcall_export.h"

namespace cstdcall{
	namespace sync{
		class CSTDCALL_API Condition{
		public:
			explicit Condition(Mutex &mutex):
			m_mutex(mutex)
			{
				InitializeConditionVariable(&m_cond);
			}

			~Condition(){}

			void wait()
			{
				SleepConditionVariableCS(&m_cond, m_mutex.getMutex(),INFINITE);
			}

			void notify()
			{
				WakeConditionVariable(&m_cond);
			}

			void notifyAll()
			{
				WakeAllConditionVariable(&m_cond);
			}

		private:
			Condition(const Condition &);
			const Condition& operator = (const Condition &);

		private:
			Mutex				     &m_mutex;
			CONDITION_VARIABLE       m_cond;

		};
	}
}

#endif