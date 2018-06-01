#ifndef EVENT_IOCP_H
#define EVENT_IOCP_H

#include <io/io_event_fd.h>
#include <errors/hht_error.h>


#include <functional>
#include <memory>
#include <map>

#ifdef WIN32
#include <windows/io_win_iocp_poller.h>
#include <WinSock2.h>
#include <Windows.h>
#include <MSWSock.h>

#endif
namespace pp {
    namespace io {

        typedef std::function<void(int)> AccpetEventHandler;

        struct io_request_t;
        class iocp_event_fd :public event_fd {
        public:
            typedef std::shared_ptr<struct io_request_t> io_request_ref;

            static const int EV_ACCPET = event_fd::EV_ERROR << 1; //only for windows
            static const int EV_WAKEUP = iocp_event_fd::EV_ACCPET << 1; //only for windows

            iocp_event_fd(event_loop *loop, int fd);
            ~iocp_event_fd() { }


            void enable_accpet(errors::error_code &error);
            void enable_wakeup(errors::error_code &error);

            void set_accpet_event_handler(const AccpetEventHandler &handler);

            void handle_event();
            void handle_accpet_event(int fd);
            int handle_read_done();
            int handle_accept_done();
            int handle_write_done();
            int handle_zero();

            int post_read(errors::error_code &error);
            int post_write(const void *data, int len, errors::error_code &error);
            int post_accpet(errors::error_code &error);


            io_request_ref remove_active_request();
            int pending_request_size();

            void set_active_pending(io_request_t *active);


            io_request_ref create_io_request(int ev);
            void queued_pending_request(const io_request_ref &request);



        private:
            iocp_event_fd(const iocp_event_fd&);
            iocp_event_fd &operator=(const iocp_event_fd&);



            AccpetEventHandler                          m_handleAccpetEvent;
            std::map<io_request_t *, io_request_ref>         ioRequestList;

            io_request_ref                                activePendingReq;
        };


#define MAX_WSA_BUFF_SIZE  (2 * (sizeof(SOCKADDR_STORAGE) + 16) + 5)
        struct io_request_t {
            io_request_t()
                :TotalBytes(0)
                , SentBytes(0)
                , IoOpt(iocp_event_fd::EV_NONE)
                , AccpetFd(-1)
                , IoFd(-1)
                , IoSize(0)
            {
                ZeroMemory(&Overlapped, sizeof(Overlapped));
                Wsabuf.buf = Buffer;
                Wsabuf.len = sizeof(Buffer);
                ZeroMemory(Wsabuf.buf, Wsabuf.len);
            }

            WSAOVERLAPPED               Overlapped;
            char                        Buffer[MAX_WSA_BUFF_SIZE];
            WSABUF                      Wsabuf;
            int                         TotalBytes;
            int                         SentBytes;
            int 			                IoOpt;
            int                         AccpetFd;
            int                         IoFd;
            int                         IoSize;  //GetQueuedCompletionStatus  second arg

        private:
            io_request_t(const io_request_t&);
            io_request_t &operator=(const io_request_t&);
        };
    }
}

#endif