#ifndef EVENT_LISTENER_H
#define EVENT_LISTENER_H

#include <functional>
#include <event_loop.h>

namespace pp {
namespace io {

class Event;
class EventListener{
public:
    typedef std::function<int (int timeoutms, EventVec &gotEvents)> ListenEventFunc;
    typedef std::function<void (Event *event)> OperateEventFunc;

    ListenEventFunc Listen;
    OperateEventFunc AddEvent;
    OperateEventFunc RemoveEvent;

};
}
}

#endif // EVENT_LISTENER_H
