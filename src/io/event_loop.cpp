#include <event_loop.h>

#include "thread_local_storage.h"
#include <event.h>
#include <event_listener.h>
#ifdef WIN32
#include "windows/event_listener_iocp.h"
#endif


#include <assert.h>
#include <iostream>
#include <memory>

using namespace std;

namespace pp {
namespace io{
    class MyTestEventListener {
    public:
        int Listen(int timeoutms, EventVec &gotEvents) {
            int ev = Event::EV_NONE;

            ev |= Event::EV_READ;
            m_eventVec[0]->SetActive(ev);
            gotEvents = m_eventVec;
            return 0;
        }

        void AddEvent(Event *ev) {
            m_eventVec.push_back(ev);
        }

        void RemoveEvent(Event *ev) {
            //m_eventVec.erase();
        }

    private:
        EventVec m_eventVec;
    };



EventLoop::EventLoop()
        :m_tid(this_thread::get_id())
        ,m_execing(false)
        ,m_eventListener(new EventListener)
{
    std::shared_ptr<MyTestEventListener> mylistener = 
        make_shared<MyTestEventListener>();
    m_eventListener->Listen = std::bind(&MyTestEventListener::Listen, mylistener,
        placeholders::_1, placeholders::_2);
    m_eventListener->AddEvent = std::bind(&MyTestEventListener::AddEvent, mylistener,
        placeholders::_1);
    m_eventListener->RemoveEvent = std::bind(&MyTestEventListener::RemoveEvent, mylistener,
        placeholders::_1);

    std::shared_ptr<IocpListener> iocplistener = make_shared<IocpListener>();
    m_eventListener->Listen = std::bind(&IocpListener::Listen, iocplistener,
        placeholders::_1, placeholders::_2);
    m_eventListener->AddEvent = std::bind(&IocpListener::AddEvent, iocplistener,
        placeholders::_1);
    m_eventListener->RemoveEvent = std::bind(&IocpListener::RemoveEvent, iocplistener,
        placeholders::_1);

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

    while(1){
        m_eventListener->Listen(INFINITE, m_activeEvents);
        for (auto ev = m_activeEvents.begin(); 
            ev != m_activeEvents.end(); ev++) {
            (*ev)->HandleEvent(NULL, 0);
        }
    }
}

void EventLoop::UpdateEvent(Event *event)
{
    assert(event->ItsLoop() == this);
    m_eventListener->AddEvent(event);
}

void EventLoop::InsertEvent(Event *event)
{
    assert(event->ItsLoop() == this);

    m_eventListener->AddEvent(event);
}

}
}
