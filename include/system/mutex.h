#ifndef HHT_MUTEX_H
#define HHT_MUTEX_H

#include <functional>

namespace pp{
	namespace system{
		class MutexPrivate;
		class  Mutex{
		public:
			Mutex();
			~Mutex();
			void lock();

			void unlock();

			void *getMutex();

		private:
			Mutex(const Mutex &);
			const Mutex& operator = (const Mutex &);

			MutexPrivate			*priv_;
		};


		class  MutexLockGuard{
		public:
			explicit MutexLockGuard(Mutex &mutex);
			~MutexLockGuard();

		private:
			MutexLockGuard(const MutexLockGuard &);
			const MutexLockGuard &  operator = (const MutexLockGuard &);

		private:
			Mutex   &mutex_;
		};

		typedef std::tr1::function<void ()> call_once_routine;
		class once_flag_private;
		class once_flag{
		public:
			once_flag();
			~once_flag();

			friend void call_once(once_flag &flag, const call_once_routine &routine);
		private:
			once_flag_private *priv_;
		};

		
		void call_once(once_flag &flag, const call_once_routine &routine);
	}
}
//#define MutexLockGuard(x) error "Missing guard object name"

#endif