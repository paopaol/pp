#ifndef socket_iocp_accpeter_H
#define socket_iocp_accpeter_H
#include <net/net_socket_accpeter.h>


#include <windows/io_win_iocp_event_fd.h> 
#include <net/net.h>
#include <errors/hht_error.h>
#include <hht.h>

namespace pp {
    namespace net {
        class io::event_loop;
        class SocketIocpAccpeter {
        public:
            SocketIocpAccpeter(int af, int type, io::event_loop *loop);
            void SetNewConnHandler(const NewConnHandler &handler);
            int Bind(const Addr &addr, errors::error_code &error);
            int Listen(errors::error_code &error);


        private:
            void handleAccpetEvent(int fd);

			DISABLE_COPY_CONSTRCT(SocketIocpAccpeter);

            io::event_loop       *m_loop;
            Socket              m_socket;
            io::iocp_event_fd     eventListenFd;
            NewConnHandler      m_handleNewConn;
            errors::error_code  m_error;
            int                 type_;
        };
    }
}

#endif