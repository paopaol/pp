#include <system/sys_thread.h>

#include <Windows.h>

namespace pp{
    namespace system{
        namespace this_thread{
            thread_id get_id()
            {
				return ::GetCurrentThreadId();
            }
        }
    }
}