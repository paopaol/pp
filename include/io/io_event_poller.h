#ifndef EVENT_LISTENER_H
#define EVENT_LISTENER_H


#include <io/io_event_loop.h>

#include <functional>
#include <errors/hht_error.h>

namespace pp {
namespace io {

class event_fd;
class event_poller{
public:
    typedef std::function<int (int timeoutms, event_fd_list &gotEventFds, errors::error_code &error)> ListenEventFunc;
    typedef std::function<void (event_fd *event, errors::error_code &error)> OperateEventFunc;
	
	event_poller(){}

    ListenEventFunc Poll;
    OperateEventFunc update_event_fd;
    OperateEventFunc remove_event_fd;
    
private:
    DISABLE_COPY_CONSTRCT(event_poller);

};
}
}

#endif // EVENT_LISTENER_H
