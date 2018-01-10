#ifndef EVENT_LOOP_H
#define EVENT_LOOP_H

#include <thread>

namespace pp{
namespace io{
class EventLoop{
public:
    EventLoop();
    ~EventLoop();
    void Exec();
private:
    bool inCreateThread();
    bool threadAlreadyExitLoop();



    std::thread::id m_tid;
    bool 			m_execing;
    bool 			m_exit;
};

}
}

#endif // EVENT_LOOP_H
