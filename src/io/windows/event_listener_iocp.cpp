#include "event_listener_iocp.h"
#include <event.h>



#include <event_listener.h>
#include <assert.h>
#include <memory>


namespace pp {
namespace io {

    static LPFN_ACCEPTEX               fnAcceptEx = NULL;

IocpListener::IocpListener()
    :m_iocp(NULL)
{
    m_iocp = create();
    assert(m_iocp != NULL);
    //fixme: add log
}

HANDLE IocpListener::create()
{
    return CreateIoCompletionPort( INVALID_HANDLE_VALUE, NULL, 0, 0);
}

int IocpListener::preparefdCtx(fdCtx *fdCtx_, int fd, int ev)
{
    fdCtx_->Fd = fd;

    fdCtx_->IoCtx.Overlapped.Internal = 0;
    fdCtx_->IoCtx .Overlapped.InternalHigh = 0;
    fdCtx_->IoCtx.Overlapped.Offset = 0;
    fdCtx_->IoCtx.Overlapped.OffsetHigh = 0;
    fdCtx_->IoCtx.Overlapped.hEvent = NULL;
    fdCtx_->IoCtx.IoOpt = ev;

    fdCtx_->IoCtx.TotalBytes = 0;
    fdCtx_->IoCtx.SentBytes = 0;
    fdCtx_->IoCtx.Wsabuf.buf = fdCtx_->IoCtx.Buffer;
    fdCtx_->IoCtx.Wsabuf.len = sizeof(fdCtx_->IoCtx.Buffer);

    ZeroMemory(fdCtx_->IoCtx.Wsabuf.buf, fdCtx_->IoCtx.Wsabuf.len);
    return 0;
}

// Create a socket with all the socket options we need, namely disable buffering
// and set linger.
//
static int createSocket(void) 
{
    int ret = 0;
    int zero = 0;
    int sd = INVALID_SOCKET;

    sd = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 
        0, WSA_FLAG_OVERLAPPED);
    if (sd == INVALID_SOCKET) {
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



int IocpListener::initAccpet(fdCtx *fdCtx_)
{
    DWORD bytes = 0;
    GUID acceptex_guid = WSAID_ACCEPTEX;

    // Load the AcceptEx extension 
    //function from the provider for this socket
    int ret = WSAIoctl(
        fdCtx_->Fd,
        SIO_GET_EXTENSION_FUNCTION_POINTER,
        &acceptex_guid,
        sizeof(acceptex_guid),
        &fnAcceptEx,
        sizeof(fnAcceptEx),
        &bytes,
        NULL,
        NULL
    );
    if (ret == SOCKET_ERROR) {
        return WSAGetLastError();
    }
    fdCtx_->IoCtx.FdAccpet = createSocket();
    if (fdCtx_->IoCtx.FdAccpet == INVALID_SOCKET) {
        return WSAGetLastError();
    }

    //
    // pay close attention to these parameters and buffer lengths
    //	
    DWORD recvBytes = 0;
    ret = fnAcceptEx(fdCtx_->Fd, fdCtx_->IoCtx.FdAccpet,
        (LPVOID)(fdCtx_->IoCtx.Buffer),
        MAX_BUFF_SIZE - (2 * (sizeof(SOCKADDR_STORAGE) + 16)),
        sizeof(SOCKADDR_STORAGE) + 16, sizeof(SOCKADDR_STORAGE) + 16,
        &recvBytes,
        (LPOVERLAPPED) &(fdCtx_->IoCtx.Overlapped));
    if (ret == SOCKET_ERROR && (ERROR_IO_PENDING != WSAGetLastError())) {
        return WSAGetLastError();
    }

    return 0;
}

void IocpListener::AddEvent(Event *_event)
{
    IocpEvent *event = static_cast<IocpEvent *>(_event);

    fdCtx *fdCtx_ = new fdCtx();
    if (event->GetTracked() & Event::EV_READ) {
        preparefdCtx(fdCtx_, event->Fd(), Event::EV_READ);
    }else if (event->GetTracked() & Event::EV_WRITE) {
        preparefdCtx(fdCtx_, event->Fd(), Event::EV_WRITE);
    } else if (event->GetTracked() & IocpEvent::EV_ACCPET) {
        preparefdCtx(fdCtx_, event->Fd(), IocpEvent::EV_ACCPET);
        initAccpet(fdCtx_);
    }

    m_iocp = CreateIoCompletionPort((HANDLE)event->Fd(), m_iocp,
                                     (DWORD_PTR)fdCtx_, 0);
    if (m_iocp == NULL) {
        if (fdCtx_){
            delete fdCtx_;
        }
        return;
    }
    m_eventsMap[fdCtx_->Fd] = event;
}

void IocpListener::RemoveEvent(Event *event)
{

}


int IocpListener::Listen(int timeoutms, EventVec &gotEvents)
{
    assert(m_iocp);

    DWORD iosize = 0;
    fdCtx *fdCtx_ = NULL;
    WSABUF bufsend;
    DWORD recvNumBytes = 0;
    DWORD sendNumBytes = 0;
    LPWSAOVERLAPPED overlapped = NULL;
    DWORD flags = 0;

    BOOL success = GetQueuedCompletionStatus(m_iocp, &iosize,
        (PDWORD_PTR)&fdCtx_, (LPOVERLAPPED *)&overlapped, timeoutms);
    if (!fdCtx_) {
        int error = GetLastError();
        //fixme:add log
        return false;
    }
    if (!success) {
        //fixme:add log
        return 0;
    }
    IocpEvent  *event = static_cast<IocpEvent *>(m_eventsMap[fdCtx_->Fd]);
    assert(event->Fd() == fdCtx_->Fd);
    if (success && iosize == 0) {
        event->SetActive(Event::EV_CLOSE);
        event->HandleEvent(NULL, 0);
        return 0;
    }
    int ret = 0;

    switch (fdCtx_->IoCtx.IoOpt) {
    case Event::EV_READ:
        event->SetActive(Event::EV_READ);
        event->HandleEvent(fdCtx_->IoCtx.Wsabuf.buf, iosize);
        break;
    case IocpEvent::EV_ACCPET:
        event->SetActive(IocpEvent::EV_ACCPET);
        event->HandleEvent(fdCtx_->IoCtx.Wsabuf.buf, iosize);
        break;
    case IocpEvent::EV_WRITE:
        event->SetActive(Event::EV_WRITE);
        //
        // a write operation has completed, determine if all the data intended to be
        // sent actually was sent.
        //
        fdCtx_->IoCtx.IoOpt = Event::EV_WRITE;
        fdCtx_->IoCtx.SentBytes += iosize;
        flags = 0;
        if (fdCtx_->IoCtx.SentBytes <
            fdCtx_->IoCtx.TotalBytes) {
            //
            // the previous write operation didn't send all the data,
            // post another send to complete the operation
            //
            bufsend.buf = fdCtx_->IoCtx.Buffer + fdCtx_->IoCtx.SentBytes;
            bufsend.len = fdCtx_->IoCtx.TotalBytes - fdCtx_->IoCtx.SentBytes;
            ret = WSASend(fdCtx_->Fd, &bufsend, 1,
                &sendNumBytes, flags, &(fdCtx_->IoCtx.Overlapped), NULL);
            if (ret == SOCKET_ERROR &&
                (ERROR_IO_PENDING != WSAGetLastError())) {
                event->SetActive(Event::EV_ERROR);
                event->HandleEvent(NULL, 0);
            }
        }
        else {
            event->SetActive(Event::EV_WRITE);
            event->HandleEvent(NULL, 0);
        }
        break;
    default:
        break;
    }
    return 0;
}


}

}
