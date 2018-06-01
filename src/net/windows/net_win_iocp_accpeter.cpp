#include "windows/net_win_iocp_accpeter.h"

#include <io/io_event_fd.h>
#include <io/io_event_loop.h>


#include <WinSock2.h>
#include <Windows.h>
#include <MSWSock.h>

using namespace std;

namespace pp {
    namespace net {
        SocketIocpAccpeter::SocketIocpAccpeter(int af, int type, io::event_loop *loop)
            :m_loop(loop)
            ,m_socket(af, type, NewSocket(af, type, m_error))
            ,type_(type)
            ,eventListenFd(loop, m_socket.fd())
        {
            assert(m_error.value() == 0);
            m_socket.SetReuseAddr(true, m_error);
            m_socket.SetNonblock(m_error);

            /** \brief   The event listen fd. set handle accpet event */
            eventListenFd.set_accpet_event_handler([&](int fd) {
                eventListenFd.remove_active_request();
                handleAccpetEvent(fd);
            });
        }

        void SocketIocpAccpeter::SetNewConnHandler(const NewConnHandler &handler) {
            m_handleNewConn = handler;
        }

        void SocketIocpAccpeter::handleAccpetEvent(int fd) {
            m_handleNewConn(fd);
        }


        int SocketIocpAccpeter::Bind(const Addr &addr, errors::error_code &error)
        {
            return m_socket.Bind(addr, error);
        }

        int SocketIocpAccpeter::Listen(errors::error_code &error)
        {
            if (type_ == SOCK_DGRAM) {
                eventListenFd.enable_read(error);
            }
            else {
                m_socket.Listen(error);
                eventListenFd.enable_accpet(error);
            }
           
            return 0;
        }
    }
}