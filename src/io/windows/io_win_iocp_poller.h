#ifndef EVENT_LISTENER_IOCP_H
#define EVENT_LISTENER_IOCP_H

#include <errors/hht_error.h>
#include <io/io_event_loop.h>

#ifdef WIN32
#include <windows/io_win_iocp_event_fd.h>

#include <WinSock2.h>
#include <Windows.h>
//#include <MSWSock.h>
#endif

#include <map>

namespace pp {
namespace io {

typedef std::map<int, event_fd *> events_map_t;

class iocp_event_fd;
class iocp_poller_private;
class iocp_poller {
public:
  iocp_poller();
  ~iocp_poller();
  int poll(int timeoutms, event_fd_list &gotEvents, errors::error_code &error);
  void update_event_fd(event_fd *event, errors::error_code &error);
  void remove_event_fd(event_fd *event, errors::error_code &error);
  void wakeup();

private:
  iocp_poller(const iocp_poller &);
  iocp_poller &operator=(iocp_poller &);

  iocp_poller_private *private_;

  // int updateEventFd(event_fd *event, errors::error_code &error);

  HANDLE m_iocp;
  // EventVec m_eventsVec;
  events_map_t events_map;
};

} // namespace io

} // namespace pp

#endif // EVENT_LISTENER_IOCP_H
