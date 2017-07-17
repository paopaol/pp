#ifndef PP_TCP_H
#define PP_TCP_H


#include <errors.h>
#include <_time.h>
#include <net.h>
#include <ip.h>

namespace pp
{
namespace net
{
class TCPAddr : public Addr
{
public:
    IP Ip;
    int Port;
    std::string Zone;
};

class TcpConn
{
  public:
    int Read(char *buffer, int len, errors::Error &e);
    int Write(char *buffer, int len, errors::Error &e);
    int Close();
    int ShutDown();
    Addr LocalAddr();
    Addr RemoteAddr();
    bool SetDeadline(const _time::Time t, errors::Error &e);
    bool SetReadline(const _time::Time t, errors::Error &e);
    bool SetWriteline(const _time::Time t, errors::Error &e);
};
}
}

#endif