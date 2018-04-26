
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
  ((result) || (GetLastError() == ERROR_IO_PENDING))

namespace pp {
    namespace io {

        static int createSocket(errors::error_code &error);



        IocpEventFd::IocpEventFd(EventLoop *loop, int fd)
            :EventFd(loop, fd)
            , m_fdCtx(new fdCtx)
            , postedWriting(false)
            ,refCnt(0)
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

        void IocpEventFd::HandleAccpetEvent(int fd, int listenfd)
        {
            if (m_handleAccpetEvent) {
                m_handleAccpetEvent(fd, listenfd);
            }
        }

        int IocpEventFd::PostRead(errors::error_code &error)
        {
            DWORD recvBytes = 0;
            DWORD flags = 0;

            int ret = WSARecv(m_fd, &m_fdCtx->ReadIoCtx.Wsabuf,
                1, &recvBytes, &flags, &m_fdCtx->ReadIoCtx.Overlapped, NULL);
            if (ret == SOCKET_ERROR) {
                int code = WSAGetLastError();
                if (code != ERROR_IO_PENDING) {
					error = hht_make_error_code(static_cast<std::errc>(code));
                    return -1;
                }
            }
            return 0;
        }


        int IocpEventFd::PostWrite(const void *data, int len, errors::error_code &error)
        {
            DWORD sentBytes = 0;

            PreparefdCtx(io::IocpEventFd::EV_WRITE);
            memcpy(m_fdCtx->WriteIoCtx.Wsabuf.buf, data, len);
            m_fdCtx->WriteIoCtx.Wsabuf.len = len;
            m_fdCtx->WriteIoCtx.TotalBytes = len;

            int ret = WSASend(m_fd, &m_fdCtx->WriteIoCtx.Wsabuf,
                1, &sentBytes, 0, &m_fdCtx->WriteIoCtx.Overlapped, NULL);
            if (ret == SOCKET_ERROR) {
                int code = WSAGetLastError();
                if (code != ERROR_IO_PENDING) {
					error = hht_make_error_code(static_cast<std::errc>(code));
                    return -1;
                }
            }
            postedWriting = true;
            return 0;
        }

        static LPFN_ACCEPTEX                fnAcceptEx = NULL;

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
                        &fnAcceptEx,
                        sizeof(fnAcceptEx),
                        &bytes,
                        NULL,
                        NULL
                    );
                });
            }
        private:
            system::once_flag            flag;
        };

        

        int IocpEventFd::PostAccpet(errors::error_code &error)
        {
            DWORD   recvBytes = 0;
            int     ret = 0;
            static IocpAccpeterFuncGetter iocpAccpeterFuncGetter(m_fdCtx->Fd);

            int accept_socket = net::NewSocket(AF_INET, SOCK_STREAM, error);
            hht_return_if_error(error, -1);

            ret = fnAcceptEx(m_fdCtx->Fd, 
                accept_socket,
                (LPVOID)(m_fdCtx->ReadIoCtx.Buffer),
                0,
                sizeof(SOCKADDR_STORAGE), 
                sizeof(SOCKADDR_STORAGE),
                &recvBytes,
                (LPOVERLAPPED) &(m_fdCtx->ReadIoCtx.Overlapped));

            if (!SUCCEEDED_WITH_IOCP(ret)) {
                error = hht_make_error_code(static_cast<std::errc>(::WSAGetLastError()));
                closesocket(accept_socket);
                return -1;
            }
            m_fdCtx->ReadIoCtx.FdAccpet = accept_socket;
            return 0;
        }


        int IocpEventFd::PreparefdCtx(int ev)
        {
            ioCtx    *ioCtxptr = NULL;

            if (ev == IocpEventFd::EV_WRITE) {
                ioCtxptr = &m_fdCtx->WriteIoCtx;
            }else {
				ioCtxptr = &m_fdCtx->ReadIoCtx;
			}


            m_fdCtx->Fd = Fd();

			memset(ioCtxptr, 0, sizeof(*ioCtxptr));
            ioCtxptr->Overlapped.Internal = 0;
            ioCtxptr->Overlapped.InternalHigh = 0;
            ioCtxptr->Overlapped.Offset = 0;
            ioCtxptr->Overlapped.OffsetHigh = 0;
            ioCtxptr->Overlapped.hEvent = NULL;
            ioCtxptr->IoOpt |= ev;

            ioCtxptr->TotalBytes = 0;
            ioCtxptr->SentBytes = 0;
            ioCtxptr->Wsabuf.buf = ioCtxptr->Buffer;
            ioCtxptr->Wsabuf.len = sizeof(ioCtxptr->Buffer);

            //ZeroMemory(ioCtxptr->Wsabuf.buf, ioCtxptr->Wsabuf.len);
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