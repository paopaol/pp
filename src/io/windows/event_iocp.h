#ifndef EVENT_IOCP_H
#define EVENT_IOCP_H

#include <event.h>

namespace pp {
    namespace io {
        class IocpEvent :public Event {
        public:
            static const int EV_ACCPET = Event::EV_ERROR << 1; //only for windows
            IocpEvent(EventLoop *loop, int fd);
            void TrackAccpet();
            void HandleEvent(void *data, int len);

            

        };
    }
}

#endif