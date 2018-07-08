#ifndef EVENT_LISTENER_H
#define EVENT_LISTENER_H

#include <io/io_event_loop.h>

#include <errors/hht_error.h>
#include <functional>

namespace pp {
namespace io {

    class event_fd;
    class event_poller {
    public:
        typedef std::function<int(int timeoutms, event_fd_list& gotEventFds,
                                  errors::error_code& error)>
            poll_handler;
        typedef std::function<void(event_fd* event, errors::error_code& error)>
                                      operate_event_handler;
        typedef std::function<void()> wakeup_handler;

        event_poller() {}

        poll_handler          poll;
        operate_event_handler update_event_fd;
        operate_event_handler remove_event_fd;
        wakeup_handler        wakeup;

    private:
        DISABLE_COPY_CONSTRCT(event_poller);
    };
}  // namespace io
}  // namespace pp

#endif  // EVENT_LISTENER_H
