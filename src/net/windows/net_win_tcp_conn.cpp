#include <net/net_tcp_conn.h>

#include <io/io_event_loop.h>
#include <windows/io_win_iocp_event_fd.h>
#include <fmt/fmt.h>
#include <bytes/buffer.h>
#include <hht.h>




#include <memory>
#include <cassert>
using namespace std;

namespace pp {
	namespace net {
		SocketConn::SocketConn(io::event_loop *loop, int af, int type, int fd)
			:evLoop(loop)
			,socket(af, type, fd)
			,evConnFd(std::make_shared<io::iocp_event_fd>(evLoop, fd))
			,closeConn(false)
			,state(Connecting)
			,remote("", -1)
			,local("", -1)
		{
			evConnFd->set_read_handler([&](errors::error_code &error) {
				handleRead(error);
			});
			evConnFd->set_write_handler([&](errors::error_code &error) {
				handleWrite(error);
			});
			evConnFd->set_close_handler([&](errors::error_code &error) {
				handleClose(error);
			});
			evConnFd->set_error_handler([&](errors::error_code &error) {
				handleError(error);
			});

			errors::error_code error;
			socket.SetNonblock(error);
		}


		void SocketConn::SetConnectHandler(const ConnectionHandler &handler)
		{
			userHandleConnect = handler;
		}

		void SocketConn::set_read_handler(const SocketMessageHandler &handler)
		{
			userHandleRead = handler;
		}

		void SocketConn::set_write_handler(const SocketMessageHandler &handler)
		{
			userHandleWrite = handler;
		}

		void SocketConn::set_close_handler(const CloseHandler &handler)
		{
			userHandleClose = handler;
		}


		//void SocketConn::set_error_handler(const Handler &handler)
		//{
		//    userHandleError = handler;
		//}

        void SocketConn::handleClose(const errors::error_code &error)
        {
            io::iocp_event_fd *evfd = static_cast<io::iocp_event_fd *>(evConnFd.get());
            evfd->remove_active_request();
            if (evfd->pending_request_size() > 0) {
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
            io::iocp_event_fd *evfd = static_cast<io::iocp_event_fd *>(evConnFd.get());

			state = DisConnected;
			if (userHandleError  && evfd->pending_request_size() == 0) {
				userHandleError(shared_from_this());
			}
		}

        void SocketConn::handleRead(errors::error_code &error)
        {
            io::iocp_event_fd *evfd = static_cast<io::iocp_event_fd *>(evConnFd.get());
           
            io::iocp_event_fd::io_request_ref doneReq = evfd->remove_active_request();

            readBuf.Write((const char *)doneReq->Buffer, doneReq->IoSize);

            if (readBuf.Len() > 0 && userHandleRead) {
                userHandleRead(shared_from_this(), readBuf, _time::Now());
            }

            if (evfd->pending_request_size() > 0) {
                return;
            }

            evfd->enable_read(error);
            if (error.value() != 0) {
                handleClose(error);
            }

        }

        void SocketConn::handleWrite(errors::error_code &error)
        {
            io::iocp_event_fd *evfd = static_cast<io::iocp_event_fd *>(evConnFd.get());
            io::iocp_event_fd::io_request_ref doneReq = evfd->remove_active_request();

            if (writeBuf.Len() > 0) {
                int minWrite = min(writeBuf.Len(), MAX_WSA_BUFF_SIZE);
                char data[MAX_WSA_BUFF_SIZE] = { 0 };
                writeBuf.Read(data, sizeof(data));

                evfd->post_write(data, minWrite, error);
                if (error.value() != 0) {
                    handleClose(error);
                }
                return;
            }
            //如果不主动投递一个接收请求，那么下次对方发送过来的数据就不会接收到了
            if (evfd->pending_request_size() > 0) {
                return;
            }
            evfd->enable_read(error);
            if (error.value() != 0) {
                handleClose(error);
            }
        }

		void SocketConn::Write(const void *data, int len)
		{
			if (state == Connected){
				Slice slice((const char *)data, (const char *)data + len);

				evLoop->run_in_loop([&, slice](){ 
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

			assert(evLoop->in_created_thread());

			io::iocp_event_fd *evfd = static_cast<io::iocp_event_fd *>(evConnFd.get());


			/*Conn 构造后,第一次发送数据的话,writeBuf
			*必然为空。HasPostedWrite必然为false,所以可以直接发送给socket，无需写入缓存,
			*如果已经有数据提交给socket的话，那么在上次提交的数据，成功发送完之前，不能
			*再次提交发送，需要先将其缓存起来。因为fdCtx的写缓冲只有一个，如果第一次发送的数据还
			*没有处理完，就发送第二次，上一次的未处理完的数据就会被覆盖掉.
			*
			* 假定第一次发送的数据很大，不能一次性发送给socket，那么就需要先发送一部分，剩下的
			*写入缓冲
			*/
			if (writeBuf.Len() > 0 && evfd->pending_request_size() == 0) {
				writeBuf.Write((char *)data, len);
				return 0;
			}

			if (len <= MAX_WSA_BUFF_SIZE) {
				evfd->post_write(data, len, error);
				return 0;
			}
			//data is too long
			evfd->post_write(data, MAX_WSA_BUFF_SIZE, error);
			writeBuf.Write((char *)data + MAX_WSA_BUFF_SIZE, len - MAX_WSA_BUFF_SIZE);
			return 0;
		}



		void SocketConn::Write(bytes::Buffer &buffer)
		{
			Slice slice;
			buffer.Read(slice);

			evLoop->run_in_loop([&, slice](){ 
				errors::error_code error;
				Write((const char *)slice.data(), slice.size(), error);
			});
		}


		int SocketConn::Close()
		{
			//state = DisConnecting;
			//CloseHandle((HANDLE)evConnFd->fd());
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

		void SocketConn::enable_read(errors::error_code &error)
		{
			evConnFd->enable_read(error);
		}

		void SocketConn::Shutdown()
		{
			if (state != Connected){
				return;
			}
			state = DisConnecting;
			evLoop->run_in_loop([&](){
				ShutdownInLoop();
			});
		}

		void SocketConn::ShutdownInLoop()
		{
			io::iocp_event_fd *evfd = static_cast<io::iocp_event_fd *>(evConnFd.get());

			assert(evLoop->in_created_thread());
            Socket::shutdownWrite(evfd->fd());
			
		}

		void SocketConn::ConnectEstablished()
		{
            io::iocp_event_fd *evfd = static_cast<io::iocp_event_fd *>(evConnFd.get());


			assert(evLoop->in_created_thread());
			assert(state == Connecting);
			state = Connected;
			if (userHandleConnect) {
				userHandleConnect(shared_from_this(), _time::Time());
			}
            evConnFd->tie(shared_from_this());
			errors::error_code error;
            evfd->enable_read(error);
            if (error.value() == 0) {
            }
		}

		void SocketConn::ConnectDestroyed()
		{
            io::iocp_event_fd *evfd = static_cast<io::iocp_event_fd *>(evConnFd.get());

            assert(evLoop->in_created_thread());

			if (state == DisConnected){
				if (userHandleConnect) {
					userHandleConnect(shared_from_this(), _time::Now());
				}
                errors::error_code error;
                evfd->remove_event(error);
			}
		}

	}
}