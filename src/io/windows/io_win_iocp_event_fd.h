#ifndef EVENT_IOCP_H
#define EVENT_IOCP_H

#include <errors/hht_error.h>
#include <io/io_event_fd.h>

#include <functional>
#include <map>
#include <memory>
#include <windows/io_win_iocp_poller.h>

#include <MSWSock.h>
#include <WinSock2.h>
#include <Windows.h>

#define SUCCEEDED_WITH_IOCP(result, ecode) \
    ((result) || ((ecode = ::WSAGetLastError()) == ERROR_IO_PENDING))

namespace pp {
namespace io {

    typedef std::function<void(errors::error_code& error)> accpet_done_handler;
    typedef std::function<void(errors::error_code& error)> connect_done_handler;
    typedef std::function<void(errors::error_code& error)> start_read_handler;
    typedef std::function<void(const char* data, int len,
                               errors::error_code& error)>
        start_write_handler;

    struct io_request_t;
    class iocp_event_fd : public event_fd {
    public:
        typedef std::shared_ptr<struct io_request_t> io_request_ref;

        static const int EV_ACCPET = event_fd::EV_ERROR
                                     << 1;  // only for windows
                                            // only for windows
        static const int EV_WAKEUP  = iocp_event_fd::EV_ACCPET << 1;
        static const int EV_CONNECT = iocp_event_fd::EV_WAKEUP << 1;

        iocp_event_fd(event_loop* loop, int fd);
        ~iocp_event_fd() {}

        void enable_accpet(const accpet_done_handler& done_handler,
                           errors::error_code&        error);
        void enable_connect(const connect_done_handler& done_handler,
                            errors::error_code&         error);
        void handle_event();
        void handle_event_with_guard();

        io_request_ref remove_active_request();
        int            pending_request_size();

        void           set_active_pending(io_request_t* active);
        io_request_ref create_io_request(int ev);
        void           queued_pending_request(const io_request_ref& request);

    private:
        iocp_event_fd(const iocp_event_fd&);
        iocp_event_fd& operator=(const iocp_event_fd&);

        int handle_read_done();
        int handle_accept_done();
        int handle_write_done();
        int handle_zero_done();
        int handle_connnect_done();

        accpet_done_handler                     handle_accpet_done_;
        connect_done_handler                    handle_connect_done_;
        start_read_handler                      start_read_;
        start_write_handler                     start_write_;
        std::map<io_request_t*, io_request_ref> io_request_list_;
        io_request_ref                          active_pending_req_;
    };

#define MAX_WSA_BUFF_SIZE (2 * (sizeof(SOCKADDR_STORAGE) + 16) + 5)
    struct io_request_t {
        io_request_t()
            : total_bytes(0)
            , sent_bytes(0)
            , IoOpt(iocp_event_fd::EV_NONE)
            , accpet_fd(-1)
            , io_fd(-1)
            , io_size(0)
        {
            ZeroMemory(&Overlapped, sizeof(Overlapped));
            wasbuf.buf = buffer;
            wasbuf.len = sizeof(buffer);
            ZeroMemory(wasbuf.buf, wasbuf.len);
        }

        WSAOVERLAPPED Overlapped;
        char          buffer[MAX_WSA_BUFF_SIZE];
        WSABUF        wasbuf;
        int           total_bytes;
        int           sent_bytes;
        int           IoOpt;
        int           accpet_fd;
        int           io_fd;
        int           io_size;  // GetQueuedCompletionStatus  second arg

    private:
        io_request_t(const io_request_t&);
        io_request_t& operator=(const io_request_t&);
    };
}  // namespace io
}  // namespace pp

#endif
