#ifndef EVENT_IOCP_H
#define EVENT_IOCP_H

#include <io/event_fd.h>
#include <errors/hht_error.h>


#include <functional>
#include <memory>

#ifdef WIN32
#include <windows/event_iocp_poller.h>
#include <WinSock2.h>
#include <Windows.h>
#include <MSWSock.h>

#endif
namespace pp {
    namespace io {

        typedef std::function<void(int, int)> AccpetEventHandler;

        class IocpEventFd :public EventFd {
        public:
            typedef std::shared_ptr<struct fdCtx> fdCtxRef;

            static const int EV_ACCPET = EventFd::EV_ERROR << 1; //only for windows
            static const int EV_WAKEUP = IocpEventFd::EV_ACCPET << 1; //only for windows

            IocpEventFd(EventLoop *loop, int fd);

            ~IocpEventFd() { }
            void EnableAccpet(errors::error_code &error);
            void DisableRead();
            void EnableWakeUp(errors::error_code &error);
            void SetAccpetEventHandler(const AccpetEventHandler &handler);
            void HandleAccpetEvent(int fd, int listenfd);

            int PostRead(errors::error_code &error);
            int PostWrite(const void *data, int len, errors::error_code &error);
            int PostAccpet(errors::error_code &error);
            int PreparefdCtx(int ev);

            bool HasPostedWrite() {
                return postedWriting;
            }
            fdCtxRef FdCtxRef() {
                return m_fdCtx;
            }
            
            //当像iocp投递了一个读请求后，在读完成之前，也有可能再次投递一个写请求
            //在两个投递都没有完成时，如果socket等关闭，GetQueuedCompletionStatus
            // 会返回两次。socket关闭意味着需要释放相关资源。为了防止double free，
            // 利用引用技术来判断资源是否还有人使用，如果为0，那么资源允许释放
            void increaseRef()
            {
                ++refCnt;
            }
            void decreaseRef()
            {
                --refCnt;
            }

            bool ZeroRef()
            {
                return refCnt == 0;
            }


        private:
			IocpEventFd(const IocpEventFd&);
			IocpEventFd &operator=(const IocpEventFd&);



            AccpetEventHandler  m_handleAccpetEvent;
            fdCtxRef            m_fdCtx;
            bool                postedWriting;
            int                 refCnt;  //should be atomic
        };
    }
}

#endif