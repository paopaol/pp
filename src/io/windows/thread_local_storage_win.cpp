#include "thread_local_storage.h"

#include <event_loop.h>

#include <Windows.h>

#include <assert.h>
#include <atomic>

using namespace std;

static DWORD index = TLS_OUT_OF_INDEXES;

void thread_local_storage_init()
{
    static atomic_bool inited = false;

    if (inited){
        return;
    }
    inited = true;
   index =  TlsAlloc();
   assert(index != TLS_OUT_OF_INDEXES);
   inited = true;
}

void loopPushToThread(void *loop)
{
   TlsSetValue(index, loop);
}

void loopPopFromThread()
{
    TlsSetValue(index, 0);
}


namespace pp{
namespace io {
bool EventLoop::threadAlreadyExistLoop()
{
    if (TlsGetValue(index) == 0){
       return false;
    }
    return true;
}
}
}
