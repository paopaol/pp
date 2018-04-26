#include <net/socket_accepter.h>
#include <net/socket_conn.h>
#include <io/event_loop.h>
#include <io/event_poller.h>
#include <windows/socket_iocp_accpeter.h>
#include <sync/chan.h>
#include <time/_time.h>

#include <iostream>
#include <string>

#include <hht.h>


using namespace pp;


static std::map<net::SocketConnRef, net::SocketConnRef> connList;




class TcpServer{
public:
	TcpServer(io::EventLoop *loop, const net::Addr &addr, const std::string &name)
		:loop_(loop)
		,accpeter_(AF_INET, SOCK_STREAM, loop)
		,bindAddr_(addr)
		,tcpServerName_(name)
	{
		accpeter_.SetNewConnHandler([&](int fd){
			OnConnection(fd);
		});
	}


	void SetOnConnectionHandler(const net::ConnectionHandler &handler)
	{
		newConntionHandler_ = handler;
	}

	void SetOnMessageHandler(const net::SocketMessageHandler &handler)
	{
		socketMessageHandler_ = handler;
	}

	bool Start(errors::error_code &error)
	{
		accpeter_.Bind(bindAddr_, error);
		hht_return_if_error(error, false);
		accpeter_.Listen(error);
		hht_return_if_error(error, false);
		return true;
	}

private:
	void OnConnection(int fd)
	{
		errors::error_code error;

		net::SocketConnRef conn = std::make_shared<net::SocketConn>(loop_, AF_INET, SOCK_STREAM, fd);
        connList_[conn->RemoteAddr(error).string()] = conn;

        int a = conn.use_count();
		conn->SetConnectHandler(newConntionHandler_);
         a = conn.use_count();
		conn->SetReadHandler(socketMessageHandler_);
         a = conn.use_count();
		conn->SetCloseHandler([&](const net::SocketConnRef &conn){
			removeFromConnList(conn);
		});
		conn->ConnectEstablished();
	}

	void removeFromConnList(const net::SocketConnRef &conn)
	{
		errors::error_code error;
		std::string remote = conn->RemoteAddr(error).string();

		auto it = connList_.find(remote);
		if (it != connList_.end()){
			connList_.erase(it);
			conn->ConnectDestroyed();
		}
	}




	io::EventLoop *loop_;
	net::SocketIocpAccpeter accpeter_;
	std::map<std::string, net::SocketConnRef> connList_;
	std::string  tcpServerName_;
	net::Addr	bindAddr_;

    net::ConnectionHandler newConntionHandler_;
	net::SocketMessageHandler socketMessageHandler_;
};




int main(int argc, char *argv)
{
    io::EventLoop loop;
    errors::error_code error;
	TcpServer server(&loop, net::Addr("0.0.0.0", 9001), "echo");

	server.SetOnConnectionHandler([&](const net::SocketConnRef &conn, const _time::Time &time){
		errors::error_code error;
		if (!conn->Connectioned()){
			std::cout << "remote:" << conn->RemoteAddr(error).string() << " closed" << std::endl;
			return;
		}
		//std::cout << time.String() << "  remote:" << conn->RemoteAddr(error).string() << "connected" << std::endl;

	});

	server.SetOnMessageHandler([&](const net::SocketConnRef &conn, bytes::Buffer &message, const _time::Time &time){
		conn->Write(message);
		conn->Shutdown();
	});

	server.Start(error);
    loop.Exec();
    //t.join();
    return 0;
}