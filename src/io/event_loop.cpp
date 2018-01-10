#include <event_loop.h>

#include "thread_local_storage.h"

#include <assert.h>

using namespace std;

namespace pp {
namespace io{
EventLoop::EventLoop()
        :m_tid(this_thread::get_id())
        ,m_execing(false)
{
    if(threadAlreadyExitLoop()){
        abort();
    }
    threadPushLoop(this);
}

EventLoop::~EventLoop()
{
    threadPopLoop();
}

bool EventLoop::inCreateThread()
{
    return m_tid == this_thread::get_id();
}

bool EventLoop::Exec()
{
    assert(!m_execing);
    assert(!inCreateThread());
    m_execing = true;

    system("pause");
}
}
}
