#include <event_loop.h>
#include <event.h>
#include <thread>

#include <winsock2.h>
#include <Windows.h>
#include <Ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <strsafe.h>


using namespace std;
using namespace pp;


void other_worker(io::EventLoop *loop)
{
    io::EventLoop app;

    io::Event ev(&app, 2);

    loop->UpdateEvent(&ev);
}

char *port = "5004";
int g_sdListen = INVALID_SOCKET;

BOOL CreateListenSocket(void) 
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);


    int nRet = 0;
    int nZero = 0;
    struct addrinfo hints = { 0 };
    struct addrinfo *addrlocal = NULL;

    //
    // Resolve the interface
    //
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_IP;

    if (getaddrinfo(NULL, port, &hints, &addrlocal) != 0) {
        int d = GetLastError();
        return(FALSE);
    }

    if (addrlocal == NULL) {
        return(FALSE);
    }

    g_sdListen = WSASocket(addrlocal->ai_family, addrlocal->ai_socktype, addrlocal->ai_protocol,
        NULL, 0, WSA_FLAG_OVERLAPPED);
    if (g_sdListen == INVALID_SOCKET) {
        return(FALSE);
    }

    nRet = ::bind(g_sdListen, addrlocal->ai_addr, (int)addrlocal->ai_addrlen);
    if (nRet == SOCKET_ERROR) {
        return(FALSE);
    }

    nRet = listen(g_sdListen, 5);
    if (nRet == SOCKET_ERROR) {
        return(FALSE);
    }

    //
    // Disable send buffering on the socket.  Setting SO_SNDBUF
    // to 0 causes winsock to stop buffering sends and perform
    // sends directly from our buffers, thereby reducing CPU usage.
    //
    // However, this does prevent the socket from ever filling the
    // send pipeline. This can lead to packets being sent that are
    // not full (i.e. the overhead of the IP and TCP headers is 
    // great compared to the amount of data being carried).
    //
    // Disabling the send buffer has less serious repercussions 
    // than disabling the receive buffer.
    //
    nZero = 0;
    nRet = setsockopt(g_sdListen, SOL_SOCKET, SO_SNDBUF, (char *)&nZero, sizeof(nZero));
    if (nRet == SOCKET_ERROR) {
        return(FALSE);
    }
    nZero = 1;
    nRet = setsockopt(g_sdListen, SOL_SOCKET, SO_REUSEADDR, (char *)&nZero, sizeof(nZero));
    if (nRet == SOCKET_ERROR) {
        return(FALSE);
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

    nRet = setsockopt(g_sdListen, SOL_SOCKET, SO_LINGER,
    (char *)&lingerStruct, sizeof(lingerStruct) );
    if( nRet == SOCKET_ERROR ) {
    myprintf("setsockopt(SO_LINGER) failed: %d\n", WSAGetLastError());
    return(FALSE);
    }
    */

    freeaddrinfo(addrlocal);

    return(TRUE);
}





int main(int argc, char *argv[])
{
    io::EventLoop app;

    CreateListenSocket();

    io::Event   event(&app, g_sdListen);
    event.SetHandleRead([&](void *data, int len) {
        printf("event fd:%d\n", event.Fd());
    });
    event.TrackRead();




    app.Exec();
}
