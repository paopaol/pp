#include <event_loop.h>

#include "thread_local_storage.h"

#include <assert.h>
#include <iostream>

using namespace std;

namespace pp {
namespace io{
EventLoop::EventLoop()
        :m_tid(this_thread::get_id())
        ,m_execing(false)
{
    thread_local_storage_init();

    assert(!threadAlreadyExistLoop());
    loopPushToThread(this);
}

EventLoop::~EventLoop()
{
   loopPopFromThread();
}

bool EventLoop::inCreateThread()
{
    return m_tid == this_thread::get_id();
}

void EventLoop::Exec()
{
    assert(!m_execing);
    assert(inCreateThread());
    m_execing = true;

}
}
}
