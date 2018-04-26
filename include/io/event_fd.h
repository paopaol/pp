#ifndef EVENT_H
#define EVENT_H


#include <bytes/buffer.h>
#include <errors/hht_error.h>
#include <hht.h>

#include <functional>
#include <vector>

#include <memory>
#include <hht.h>


namespace pp {
    namespace io {

        class EventLoop;
        class EventFd {
        public:
            static const int    EV_NONE = 0;
            static const int    EV_READ = 1;
            static const int    EV_WRITE = EV_READ << 2;
            static const int    EV_CLOSE = EV_WRITE << 1;
            static const int    EV_ERROR = EV_CLOSE << 1;

            typedef std::function<void(void *data, int len)> DataHandler;
            typedef std::function<void(void *data, int len)> BufferHandler;
            typedef std::function<void()> Handler;

            EventFd(EventLoop *loop, int fd);
            ~EventFd() {}
            void SetWriteHandler(const DataHandler &handler);
            void SetReadHandler(const DataHandler &handler);
            void SetErrorHandler(const Handler &handler);
            void SetCloseHandler(const Handler &handler);

            void HandleEvent(void *data, int len);
            void HandleEventWithGuard(void *data, int len);
            void SetActive(int events);

            void EnableRead(errors::error_code &error);
           
            void EnableWrite(errors::error_code &error);
            int  GetEnabledEvent();
            bool HasBeenEnableRead();

            inline int Fd() { return m_fd; };
            void tie(const std::shared_ptr<void> &obj) {
                tie_ = obj;
                tied_ = true;
            }

            EventLoop *ItsLoop();

            void updateEvent(errors::error_code &error);
			void removeEvent(errors::error_code &error);
            
       

            EventLoop *m_eventLoop;
            int m_fd;
            int m_enableEvent;
            bool enableRead;
            bool enableWrite;
            int m_activeEvent;
            DataHandler m_handleWrite;
            DataHandler m_handleRead;
            Handler m_handleClose;
            Handler m_handleError;
            std::weak_ptr<void> tie_;
            bool                tied_;

		private:
            DISABLE_COPY_CONSTRCT(EventFd);
        };

        typedef std::shared_ptr<EventFd>  EventFdRef;


    }
}

#endif // EVENT_H
