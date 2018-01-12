
#include <event.h>

#include <memory>

namespace pp {
    namespace io {
        class Conn {
        private:
            int handleRead(void *data, int len);

            std::shared_ptr<Event> m_event;
        };
    }
}