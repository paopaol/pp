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

            typedef std::function<void(errors::error_code &error)> EventHandler;

            EventFd(EventLoop *loop, int fd);
            ~EventFd() {}
            void SetWriteHandler(const EventHandler &handler);
            void SetReadHandler(const EventHandler &handler);
            void SetErrorHandler(const EventHandler &handler);
            void SetCloseHandler(const EventHandler &handler);

            void HandleEvent();
            void HandleEventWithGuard();
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
            EventHandler m_handleWrite;
            EventHandler m_handleRead;
            EventHandler m_handleClose;
            EventHandler m_handleError;
            std::weak_ptr<void> tie_;
            bool                tied_;

		private:
            DISABLE_COPY_CONSTRCT(EventFd);
        };

        typedef std::shared_ptr<EventFd>  EventFdRef;


    }
}

#endif // EVENT_H
