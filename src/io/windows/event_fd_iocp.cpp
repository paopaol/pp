
#ifdef WIN32
#include "windows/event_fd_iocp.h"
#endif

#include <io/event_loop.h>

#include <memory>
//#include <mutex>
#include <system/mutex.h>
#include <net/net.h>

using namespace std;


#define SUCCEEDED_WITH_IOCP(result)                                  \
  ((result) || ((::WSAGetLastError()) == ERROR_IO_PENDING))

namespace pp {
    namespace io {

        static int createSocket(errors::error_code &error);



        IocpEventFd::IocpEventFd(EventLoop *loop, int fd)
            :EventFd(loop, fd)
        {
        }

        void IocpEventFd::DisableRead()
        {
            enableRead = false;
        }
        void IocpEventFd::EnableAccpet(errors::error_code &error)
        {
            m_enableEvent |= IocpEventFd::EV_ACCPET;
            m_eventLoop->UpdateEventFd(this, error);
        }

        void IocpEventFd::EnableWakeUp(errors::error_code &error)
        {
            m_enableEvent |= IocpEventFd::EV_WAKEUP;
            m_eventLoop->UpdateEventFd(this, error);
        }

        void IocpEventFd::SetAccpetEventHandler(const AccpetEventHandler &handler)
        {
            m_handleAccpetEvent = handler;
        }

        void IocpEventFd::HandleAccpetEvent(int fd)
        {
            if (m_handleAccpetEvent) {
                m_handleAccpetEvent(fd);
            }
        }

        int IocpEventFd::PostRead(errors::error_code &error)
        {
            DWORD recvBytes = 0;
            DWORD flags = 0;

            IoRequestRef request = createIoRequest(EventFd::EV_READ);
            int ret = WSARecv(m_fd, &request.get()->Wsabuf,
                1, &recvBytes, &flags, &request.get()->Overlapped, NULL);
            if (!SUCCEEDED_WITH_IOCP(ret == 0)) {
                int code = ::GetLastError();
                error = hht_make_error_code(static_cast<std::errc>(code));
                return -1;
            }
            queuedPendingRequest(request);
            return 0;
        }


        int IocpEventFd::handleZero()
        {
            SetActive(IocpEventFd::EV_CLOSE);
            EventFd::HandleEvent();
            return 0;
        }

        int IocpEventFd::handleReadDone()
        {
            assert(activePendingReq != nullptr);

            if (activePendingReq->IoSize == 0) {
                handleZero();
                return 0;
            }
            SetActive(IocpEventFd::EV_READ);
            EventFd::HandleEvent();
            return 0;
        }



        int IocpEventFd::handleAcceptDone()
        {
            errors::error_code error;

            int acpepetFd = activePendingReq->AccpetFd;
            int listenFd = activePendingReq->IoFd;

            int ret = ::setsockopt(
                acpepetFd,
                SOL_SOCKET,
                SO_UPDATE_ACCEPT_CONTEXT,
                (char *)&activePendingReq->IoFd,
                sizeof(activePendingReq->IoFd)
            );

            SetActive(IocpEventFd::EV_ACCPET);
            HandleAccpetEvent(acpepetFd);
           
            EnableAccpet(error);

            return 0;
        }

        int IocpEventFd::handleWriteDone()
        {
            errors::error_code error;

            assert(activePendingReq != nullptr);
            if (activePendingReq->IoSize == 0) {
                handleZero();
                return 0;
            }
            assert(activePendingReq != nullptr);
            activePendingReq->IoOpt = IocpEventFd::EV_WRITE;
            activePendingReq->SentBytes += activePendingReq->IoSize;
            if (activePendingReq->SentBytes < activePendingReq->TotalBytes) {
                PostWrite(activePendingReq->Buffer + activePendingReq->SentBytes,
                    activePendingReq->TotalBytes - activePendingReq->SentBytes, error);
            }
            else {
                SetActive(IocpEventFd::EV_WRITE);
                EventFd::HandleEvent();
            }
            return 0;
        }





        void IocpEventFd::HandleEvent()
        {
            if (activePendingReq->IoOpt & IocpEventFd::EV_WAKEUP) {
                //handleWakeUp();
                return ;
            }
            else if (activePendingReq->IoOpt & IocpEventFd::EV_READ) {
                handleReadDone();
            }
            else if (activePendingReq->IoOpt & IocpEventFd::EV_ACCPET) {
                handleAcceptDone();
            }
            else if (activePendingReq->IoOpt & IocpEventFd::EV_WRITE) {
                handleWriteDone();
            }
        }

        int IocpEventFd::PostWrite(const void *data, int len, errors::error_code &error)
        {
            DWORD sentBytes = 0;

            IoRequestRef request = createIoRequest(IocpEventFd::EV_WRITE);
            memcpy(request.get()->Wsabuf.buf, data, len);
            request.get()->Wsabuf.len = len;
            request.get()->TotalBytes = len;

            int ret = WSASend(m_fd, &request.get()->Wsabuf,
                1, &sentBytes, 0, &request.get()->Overlapped, NULL);
            if (!SUCCEEDED_WITH_IOCP(ret == 0)) {
                error = hht_make_error_code(static_cast<std::errc>(::WSAGetLastError()));
                return -1;
            }
            queuedPendingRequest(request);
            return 0;
        }

        static LPFN_ACCEPTEX                acceptEx = NULL;

        class IocpAccpeterFuncGetter {
        public:
            IocpAccpeterFuncGetter(int fd) {
                int ret = 0;
                DWORD                               bytes = 0;
                GUID                                acceptex_guid = WSAID_ACCEPTEX;
                system::call_once(flag, [&] {
                    ret = WSAIoctl(
                        fd,
                        SIO_GET_EXTENSION_FUNCTION_POINTER,
                        &acceptex_guid,
                        sizeof(acceptex_guid),
                        &acceptEx,
                        sizeof(acceptEx),
                        &bytes,
                        NULL,
                        NULL
                    );
                });
            }
        private:
            system::once_flag            flag;
        };

        
        IocpEventFd::IoRequestRef IocpEventFd::createIoRequest(int ev)
        {
            IoRequestRef    request = std::make_shared<struct IoRequest>();

            request->IoFd = Fd();
            request->IoOpt |= ev;
            return request;
        }


        int IocpEventFd::PostAccpet(errors::error_code &error)
        {
            DWORD   recvBytes = 0;
            int     ret = 0;
            static IocpAccpeterFuncGetter iocpAccpeterFuncGetter(Fd());

            int accept_socket = net::NewSocket(AF_INET, SOCK_STREAM, error);
            hht_return_if_error(error, -1);

            IoRequestRef    request = createIoRequest(IocpEventFd::EV_ACCPET);

            ret = acceptEx(Fd(),
                accept_socket,
                (LPVOID)(request->Buffer),
                0,
                sizeof(SOCKADDR_STORAGE), 
                sizeof(SOCKADDR_STORAGE),
                &recvBytes,
                (LPOVERLAPPED) &(request->Overlapped));

            if (!SUCCEEDED_WITH_IOCP(ret)) {
                error = hht_make_error_code(static_cast<std::errc>(::WSAGetLastError()));
                closesocket(accept_socket);
                return -1;
            }
            queuedPendingRequest(request);
            request->AccpetFd = accept_socket;
            return 0;
        }







        // Create a socket with all the socket options we need, namely disable buffering
        // and set linger.
        //
        static int createSocket(errors::error_code &error)
        {
            int ret = 0;
            int zero = 0;
            int sd = INVALID_SOCKET;

            sd = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL,
                0, WSA_FLAG_OVERLAPPED);
            if (sd == INVALID_SOCKET) {
				error = hht_make_error_code(static_cast<std::errc>(WSAGetLastError()));
                return(sd);
            }

            //
            // Disable send buffering on the socket.  Setting SO_SNDBUF
            // to 0 causes winsock to stop buffering sends and perform
            // sends directly from our buffers, thereby save one memory copy.
            //
            // However, this does prevent the socket from ever filling the
            // send pipeline. This can lead to packets being sent that are
            // not full (i.e. the overhead of the IP and TCP headers is 
            // great compared to the amount of data being carried).
            //
            // Disabling the send buffer has less serious repercussions 
            // than disabling the receive buffer.
            //
            zero = 0;
            ret = setsockopt(sd, SOL_SOCKET, SO_SNDBUF, (char *)&zero, sizeof(zero));
            if (ret == SOCKET_ERROR) {
				error = hht_make_error_code(static_cast<std::errc>(WSAGetLastError()));
                return(sd);
            }

            //
            // Don't disable receive buffering. This will cause poor network
            // performance since if no receive is posted and no receive buffers,
            // the TCP stack will set the window size to zero and the peer will
            // no longer be allowed to send data.
            //

            // 
            // Do not set a linger value...especially don't set it to an abortive
            // close. If you set abortive close and there happens to be a bit of
            // data remaining to be transfered (or data that has not been 
            // acknowledged by the peer), the connection will be forcefully reset
            // and will lead to a loss of data (i.e. the peer won't get the last
            // bit of data). This is BAD. If you are worried about malicious
            // clients connecting and then not sending or receiving, the server
            // should maintain a timer on each connection. If after some point,
            // the server deems a connection is "stale" it can then set linger
            // to be abortive and close the connection.
            //

            /*
            LINGER lingerStruct;

            lingerStruct.l_onoff = 1;
            lingerStruct.l_linger = 0;
            nRet = setsockopt(sdSocket, SOL_SOCKET, SO_LINGER,
            (char *)&lingerStruct, sizeof(lingerStruct));
            if( nRet == SOCKET_ERROR ) {
            myprintf("setsockopt(SO_LINGER) failed: %d\n", WSAGetLastError());
            return(sdSocket);
            }
            */

            return(sd);
        }

    }
}