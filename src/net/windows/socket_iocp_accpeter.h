#ifndef socket_iocp_accpeter_H
#define socket_iocp_accpeter_H
#include <net/socket_accepter.h>


#include <windows/event_fd_iocp.h> 
#include <net/net.h>
#include <errors/hht_error.h>
#include <hht.h>

namespace pp {
    namespace net {
        class io::EventLoop;
        class SocketIocpAccpeter {
        public:
            SocketIocpAccpeter(int af, int type, io::EventLoop *loop);
            void SetNewConnHandler(const NewConnHandler &handler);
            int Bind(const Addr &addr, errors::error_code &error);
            int Listen(errors::error_code &error);


        private:
            void handleAccpetEvent(int fd, int listenfd);

			DISABLE_COPY_CONSTRCT(SocketIocpAccpeter);

            io::EventLoop       *m_loop;
            Socket              m_socket;
            io::IocpEventFd     eventListenFd;
            NewConnHandler      m_handleNewConn;
            errors::error_code  m_error;
            int                 type_;
        };
    }
}

#endif