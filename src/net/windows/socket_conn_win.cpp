#include <net/socket_conn.h>

#include <io/event_loop.h>
#include <windows/event_fd_iocp.h>
#include <fmt/fmt.h>
#include <bytes/buffer.h>
#include <hht.h>




#include <memory>
#include <cassert>
using namespace std;

namespace pp {
	namespace net {
		SocketConn::SocketConn(io::EventLoop *loop, int af, int type, int fd)
			:evLoop(loop)
			,socket(af, type, fd)
			,evConnFd(std::make_shared<io::IocpEventFd>(evLoop, fd))
			,closeConn(false)
			,state(Connecting)
			,remote("", -1)
			,local("", -1)
		{
			evConnFd->SetReadHandler([&](errors::error_code &error) {
				handleRead(error);
			});
			evConnFd->SetWriteHandler([&](errors::error_code &error) {
				handleWrite(error);
			});
			evConnFd->SetCloseHandler([&](errors::error_code &error) {
				handleClose(error);
			});
			evConnFd->SetErrorHandler([&](errors::error_code &error) {
				handleError(error);
			});

			errors::error_code error;
			socket.SetNonblock(error);
		}


		void SocketConn::SetConnectHandler(const ConnectionHandler &handler)
		{
			userHandleConnect = handler;
		}

		void SocketConn::SetReadHandler(const SocketMessageHandler &handler)
		{
			userHandleRead = handler;
		}

		void SocketConn::SetWriteHandler(const SocketMessageHandler &handler)
		{
			userHandleWrite = handler;
		}

		void SocketConn::SetCloseHandler(const CloseHandler &handler)
		{
			userHandleClose = handler;
		}


		//void SocketConn::SetErrorHandler(const Handler &handler)
		//{
		//    userHandleError = handler;
		//}

        void SocketConn::handleClose(const errors::error_code &error)
        {
            io::IocpEventFd *evfd = static_cast<io::IocpEventFd *>(evConnFd.get());
            evfd->removeActiveRequest();
            if (evfd->PendingRequestSize() > 0) {
                return;
            }

            assert(state == Connected || state == DisConnecting);
            state = DisConnected;
           
            if (userHandleClose) {
                userHandleClose(shared_from_this());
            }
        }

		void SocketConn::handleError(const errors::error_code &error)
		{
            io::IocpEventFd *evfd = static_cast<io::IocpEventFd *>(evConnFd.get());

			state = DisConnected;
			if (userHandleError  && evfd->PendingRequestSize() == 0) {
				userHandleError(shared_from_this());
			}
		}

        void SocketConn::handleRead(errors::error_code &error)
        {
            io::IocpEventFd *evfd = static_cast<io::IocpEventFd *>(evConnFd.get());
           
            io::IocpEventFd::IoRequestRef doneReq = evfd->removeActiveRequest();

            readBuf.Write((const char *)doneReq->Buffer, doneReq->IoSize);

            if (readBuf.Len() > 0 && userHandleRead) {
                userHandleRead(shared_from_this(), readBuf, _time::Now());
            }

            if (evfd->PendingRequestSize() > 0) {
                return;
            }

            evfd->EnableRead(error);
            if (error.value() != 0) {
                handleClose(error);
            }

        }

        void SocketConn::handleWrite(errors::error_code &error)
        {
            io::IocpEventFd *evfd = static_cast<io::IocpEventFd *>(evConnFd.get());
            io::IocpEventFd::IoRequestRef doneReq = evfd->removeActiveRequest();

            if (writeBuf.Len() > 0) {
                int minWrite = min(writeBuf.Len(), MAX_WSA_BUFF_SIZE);
                char data[MAX_WSA_BUFF_SIZE] = { 0 };
                writeBuf.Read(data, sizeof(data));

                evfd->PostWrite(data, minWrite, error);
                if (error.value() != 0) {
                    handleClose(error);
                }
                return;
            }
            //如果不主动投递一个接收请求，那么下次对方发送过来的数据就不会接收到了
            if (evfd->PendingRequestSize() > 0) {
                return;
            }
            evfd->EnableRead(error);
            if (error.value() != 0) {
                handleClose(error);
            }
        }

		void SocketConn::Write(const void *data, int len)
		{
			if (state == Connected){
				Slice slice((const char *)data, (const char *)data + len);

				evLoop->RunInLoop([&, slice](){ 
					errors::error_code error;
					Write(slice.data(), slice.size(), error);
				});
			}
		}


		int SocketConn::Write(const void *data, int len, errors::error_code &error)
		{
			if (state != Connected) {
				return 0;
			}

			assert(evLoop->InCreateThread());

			io::IocpEventFd *evfd = static_cast<io::IocpEventFd *>(evConnFd.get());


			/*Conn 构造后,第一次发送数据的话,writeBuf
			*必然为空。HasPostedWrite必然为false,所以可以直接发送给socket，无需写入缓存,
			*如果已经有数据提交给socket的话，那么在上次提交的数据，成功发送完之前，不能
			*再次提交发送，需要先将其缓存起来。因为fdCtx的写缓冲只有一个，如果第一次发送的数据还
			*没有处理完，就发送第二次，上一次的未处理完的数据就会被覆盖掉.
			*
			* 假定第一次发送的数据很大，不能一次性发送给socket，那么就需要先发送一部分，剩下的
			*写入缓冲
			*/
			if (writeBuf.Len() > 0 && evfd->PendingRequestSize() == 0) {
				writeBuf.Write((char *)data, len);
				return 0;
			}

			if (len <= MAX_WSA_BUFF_SIZE) {
				evfd->PostWrite(data, len, error);
				return 0;
			}
			//data is too long
			evfd->PostWrite(data, MAX_WSA_BUFF_SIZE, error);
			writeBuf.Write((char *)data + MAX_WSA_BUFF_SIZE, len - MAX_WSA_BUFF_SIZE);
			//TrackWrite();
			return 0;
		}



		void SocketConn::Write(bytes::Buffer &buffer)
		{
			Slice slice;
			buffer.Read(slice);

			evLoop->RunInLoop([&, slice](){ 
				errors::error_code error;
				Write((const char *)slice.data(), slice.size(), error);
			});
		}


		int SocketConn::Close()
		{
			//state = DisConnecting;
			//CloseHandle((HANDLE)evConnFd->Fd());
			//if (userHandleClose) {
			//    userHandleClose();
			//}

			return 0;
		}

		Addr SocketConn::RemoteAddr(errors::error_code &error)
		{
			if (!remote.Ip.empty()) {
				return remote;
			}

			remote = socket.RemoteAddr(error);
			return remote;
		}

		Addr SocketConn::LocalAddr(errors::error_code &error)
		{
			if (!local.Ip.empty()) {
				return remote;
			}
			local = socket.LocalAddr(error);
			return local;
		}

		void SocketConn::EnableRead(errors::error_code &error)
		{
			evConnFd->EnableRead(error);
		}

		void SocketConn::Shutdown()
		{
			if (state != Connected){
				return;
			}
			state = DisConnecting;
			evLoop->RunInLoop([&](){
				ShutdownInLoop();
			});
		}

		void SocketConn::ShutdownInLoop()
		{
			io::IocpEventFd *evfd = static_cast<io::IocpEventFd *>(evConnFd.get());

			assert(evLoop->InCreateThread());
            Socket::shutdownWrite(evfd->Fd());
			
		}

		void SocketConn::ConnectEstablished()
		{
            io::IocpEventFd *evfd = static_cast<io::IocpEventFd *>(evConnFd.get());


			assert(evLoop->InCreateThread());
			assert(state == Connecting);
			state = Connected;
			if (userHandleConnect) {
				userHandleConnect(shared_from_this(), _time::Time());
			}
            evConnFd->tie(shared_from_this());
			errors::error_code error;
            evfd->EnableRead(error);
            if (error.value() == 0) {
            }
		}

		void SocketConn::ConnectDestroyed()
		{
            io::IocpEventFd *evfd = static_cast<io::IocpEventFd *>(evConnFd.get());

            assert(evLoop->InCreateThread());

			if (state == DisConnected){
				if (userHandleConnect) {
					userHandleConnect(shared_from_this(), _time::Now());
				}
                errors::error_code error;
                evfd->removeEvent(error);
			}
		}

	}
}