
#include <windows.h>
#include <functional>
#include <system/sys_mutex.h>

namespace pp{
	namespace system{
		class MutexPrivate{
		public:
			MutexPrivate(){
				InitializeCriticalSectionAndSpinCount(&cs_, 0);

			}
			~MutexPrivate(){
				DeleteCriticalSection(&cs_);
			}
			void lock(){
				EnterCriticalSection(&cs_);
			}
			void unlock(){
				LeaveCriticalSection(&cs_);
			}

			CRITICAL_SECTION *getMutex(){
				return &cs_;
			}
		private:
			MutexPrivate(const MutexPrivate&);
			MutexPrivate &operator=(const MutexPrivate &);


			CRITICAL_SECTION        cs_;
		};


		Mutex::Mutex()
			:priv_(new MutexPrivate)
		{
			//InitializeCriticalSectionAndSpinCount(&m_cs, 0);
		}
		Mutex::~Mutex()
		{
			delete priv_;
			//DeleteCriticalSection(&m_cs);
		}

		void Mutex::lock(){
			priv_->lock();
		}

		void Mutex::unlock(){
			priv_->unlock();
		}

		void *Mutex::getMutex(){
			return (void *)priv_->getMutex();
		}




		MutexLockGuard::MutexLockGuard(Mutex &mutex)
			:mutex_(mutex)
		{
			mutex_.lock();
		}

		MutexLockGuard::~MutexLockGuard()
		{
			mutex_.unlock();    
		}


		class once_flag_private{
		public:
			once_flag_private(){
				InitOnceInitialize(&init_once_);
			}
			~once_flag_private(){
			}
			PINIT_ONCE get_init_once(){
				return &init_once_;
			}
		
		private:
			once_flag_private(const once_flag_private &);
			once_flag_private &operator=(const once_flag_private &);

			INIT_ONCE init_once_;
		};

		once_flag::once_flag()
			:priv_(new once_flag_private())
		{
		}

		once_flag::~once_flag()
		{
			delete priv_;
		}


		// Initialization callback function that creates the event object 
		static BOOL CALLBACK InitHandleFunction (
			PINIT_ONCE initonce,        // Pointer to one-time initialization structure        
			PVOID parameter,            // Optional parameter passed by InitOnceExecuteOnce            
			PVOID *ctx)           // Receives pointer to event object           
		{
			if(parameter){
				call_once_routine *fn = (call_once_routine *)parameter;
				(*fn)();
			}
			return TRUE;
		}
		void call_once(once_flag &flag, const call_once_routine &routine)
		{
			PVOID lpContext;

			InitOnceExecuteOnce(flag.priv_->get_init_once(), 
				InitHandleFunction, (PVOID)&routine, &lpContext);
		}

	}

}