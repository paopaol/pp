#ifndef EVENT_LOOP_H
#define EVENT_LOOP_H

#include <thread>
#include <vector>

namespace pp{
namespace io{


class Event;
class EventListener;
typedef std::vector<Event *> EventVec;
class EventLoop{
public:
    EventLoop();
    ~EventLoop();
    void Exec();

    void InsertEvent(Event *event);
    void UpdateEvent(Event *event);

private:
    bool inCreateThread();
    bool threadAlreadyExistLoop();



    std::thread::id m_tid;
    bool 			m_execing;
    bool 			m_exit;
    EventListener   *m_eventListener;
    EventVec  		m_activeEvents;
};

}
}

#endif // EVENT_LOOP_H
