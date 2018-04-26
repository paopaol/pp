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
            
            //����iocpͶ����һ����������ڶ����֮ǰ��Ҳ�п����ٴ�Ͷ��һ��д����
            //������Ͷ�ݶ�û�����ʱ�����socket�ȹرգ�GetQueuedCompletionStatus
            // �᷵�����Ρ�socket�ر���ζ����Ҫ�ͷ������Դ��Ϊ�˷�ֹdouble free��
            // �������ü������ж���Դ�Ƿ�����ʹ�ã����Ϊ0����ô��Դ�����ͷ�
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