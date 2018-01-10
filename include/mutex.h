#ifndef CSTDCALL_MUTEX_H
#define CSTDCALL_MUTEX_H

#include <windows.h>
#include "cstdcall_export.h"

namespace cstdcall{
	namespace sync{
		class CSTDCALL_API Mutex{
		public:
			Mutex(){
				InitializeCriticalSectionAndSpinCount(&m_cs, 0);
			}
			~Mutex(){
				DeleteCriticalSection(&m_cs);
			}

			void lock(){
				EnterCriticalSection(&m_cs);
			}

			void unlock(){
				LeaveCriticalSection(&m_cs);
			}

			CRITICAL_SECTION *getMutex(){
				return &m_cs;
			}

		private:
			Mutex(const Mutex &);
			const Mutex& operator = (const Mutex &);
		private:

			CRITICAL_SECTION        m_cs;
		};


		class CSTDCALL_API MutexLockGuard{
		public:
			explicit MutexLockGuard(Mutex &mutex):
			m_cs(mutex)
			{
				m_cs.lock();
			}
			~MutexLockGuard()
			{
				m_cs.unlock();    
			}

		private:
			MutexLockGuard(const MutexLockGuard &);
			const MutexLockGuard &  operator = (const MutexLockGuard &);

		private:
			Mutex   &m_cs;
		};
	}
}
#define MutexLockGuard(x) error "Missing guard object name"

#endif