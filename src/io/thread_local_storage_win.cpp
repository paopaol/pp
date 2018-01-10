#include "thread_local_storage.h"

#include <event_loop.h>

#include <Windows.h>

#include <assert.h>

static DWORD index = TLS_OUT_OF_INDEXES;

void threadPushLoop(void *loop)
{
   index =  TlsAlloc();
   assert(index == TLS_OUT_OF_INDEXES);

   TlsSetValue(index, loop);
}

void threadPopLoop()
{
    TlsFree(index);
}


namespace pp{
namespace io {
bool EventLoop::threadAlreadyExitLohop()
{
    assert(TlsGetValue(index) == 0);
}
}
}
