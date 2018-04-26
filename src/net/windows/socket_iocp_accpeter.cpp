#include <windows/socket_iocp_accpeter.h>

#include <io/event_fd.h>
#include <io/event_loop.h>


#include <WinSock2.h>
#include <Windows.h>
#include <MSWSock.h>

using namespace std;

namespace pp {
    namespace net {
        SocketIocpAccpeter::SocketIocpAccpeter(int af, int type, io::EventLoop *loop)
            :m_loop(loop)
            ,m_socket(af, type, NewSocket(af, type, m_error))
            ,type_(type)
            ,eventListenFd(loop, m_socket.Fd())
        {
            assert(m_error.value() == 0);
            m_socket.SetReuseAddr(true, m_error);
            m_socket.SetNonblock(m_error);

            /** \brief   The event listen fd. set handle accpet event */
            eventListenFd.SetAccpetEventHandler([&](int fd, int listenfd) {
                handleAccpetEvent(fd, listenfd);
            });
        }

        void SocketIocpAccpeter::SetNewConnHandler(const NewConnHandler &handler) {
            m_handleNewConn = handler;
        }

        void SocketIocpAccpeter::handleAccpetEvent(int fd, int listenfd) {
            int ret = ::setsockopt(
                fd,
                SOL_SOCKET,
                SO_UPDATE_ACCEPT_CONTEXT,
                (char *)&listenfd,
                sizeof(listenfd)
            );
            m_handleNewConn(fd);
        }


        int SocketIocpAccpeter::Bind(const Addr &addr, errors::error_code &error)
        {
            return m_socket.Bind(addr, error);
        }

        int SocketIocpAccpeter::Listen(errors::error_code &error)
        {
            if (type_ == SOCK_DGRAM) {
                eventListenFd.EnableRead(error);
            }
            else {
                m_socket.Listen(error);
                eventListenFd.EnableAccpet(error);
            }
           
            return 0;
        }
    }
}