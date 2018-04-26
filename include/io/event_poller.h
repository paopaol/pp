#ifndef EVENT_LISTENER_H
#define EVENT_LISTENER_H


#include <io/event_loop.h>

#include <functional>
#include <errors/hht_error.h>

namespace pp {
namespace io {

class EventFd;
class EventPoller{
public:
    typedef std::function<int (int timeoutms, EventFdList &gotEventFds, errors::error_code &error)> ListenEventFunc;
    typedef std::function<void (EventFd *event, errors::error_code &error)> OperateEventFunc;
	
	EventPoller(){}

    ListenEventFunc Poll;
    OperateEventFunc UpdateEventFd;
    OperateEventFunc RemoveEventFd;
    
private:
    DISABLE_COPY_CONSTRCT(EventPoller);

};
}
}

#endif // EVENT_LISTENER_H
