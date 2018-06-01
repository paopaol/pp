#ifndef NET_CONN_H
#define NET_CONN_H

#include <io/io_event_fd.h>
#include <bytes/buffer.h>
#include <net/net.h>
#include <time/_time.h>

#include <memory>



namespace pp {
    namespace net {
        class SocketConn;
		typedef std::shared_ptr<SocketConn>  SocketConnRef;
        typedef std::function<void(const net::SocketConnRef &conn)> Handler;
        typedef std::function<void(const net::SocketConnRef &conn)> CloseHandler;

        typedef std::function<void(const net::SocketConnRef, const _time::Time &)> ConnectionHandler;
        typedef std::function<void(const net::SocketConnRef, bytes::Buffer &, const _time::Time &)> SocketMessageHandler;

        class io::event_loop;
        class SocketConn: public std::enable_shared_from_this<SocketConn> {
        public:
            enum ConnStatus {
                Connecting,
                Connected,
                DisConnecting,
                DisConnected
            };


            SocketConn(io::event_loop *loop,int af, int type, int fd);
            ~SocketConn() {
            }
            void SetConnectHandler(const ConnectionHandler &handler);
            void set_read_handler(const SocketMessageHandler &handler);
            void set_write_handler(const SocketMessageHandler &handler);
            //void set_error_handler(const Handler &handler);
            void set_close_handler(const CloseHandler &handler);
       
            void Write(const void *data, int len);
            void Write(bytes::Buffer &buffer);

            int Close();  
            void Shutdown();
            void ConnectEstablished();
			void ConnectDestroyed();
			bool Connectioned(){return state == Connected;}
            Addr RemoteAddr(errors::error_code &error);
            Addr LocalAddr(errors::error_code &error);
			void enable_read(errors::error_code &error);

        private:
            void handleRead(errors::error_code &error);
            void handleWrite(errors::error_code &error);
            void handleClose(const errors::error_code &error);
            void handleError(const errors::error_code &error);
			int Write(const void *data, int len, errors::error_code &error);

			void ShutdownInLoop();


            DISABLE_COPY_CONSTRCT(SocketConn);


            io::event_loop *evLoop;
            net::Socket    socket;
            io::event_fd_ref     evConnFd;
            SocketMessageHandler userHandleRead;
            SocketMessageHandler userHandleWrite;
            CloseHandler userHandleClose;
            Handler userHandleError;
            ConnectionHandler userHandleConnect;
            bytes::Buffer      readBuf;
            bytes::Buffer      writeBuf;
            bool               postedWriting;
            bool               closeConn;
            ConnStatus         state;
            Addr            remote;
            Addr            local;
        };
    }
}

#endif