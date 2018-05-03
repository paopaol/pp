#include <io/event_fd.h>

#include <io/event_loop.h>

namespace pp {
    namespace io {
        EventFd::EventFd(EventLoop *loop, int fd)
            :m_eventLoop(loop)
            , m_fd(fd)
            , m_enableEvent(EV_NONE)
            , m_activeEvent(EV_NONE)
            , enableRead(false)
            , enableWrite(false)
            , tied_(false)
        {
        }

        void EventFd::SetWriteHandler(const EventHandler &handler)
        {
            m_handleWrite = handler;
        }

        void EventFd::SetReadHandler(const EventHandler &handler)
        {
            m_handleRead = handler;
        }

        void EventFd::SetErrorHandler(const EventHandler &handler)
        {
           
			 m_handleError = handler;
        }

        void EventFd::SetCloseHandler(const EventHandler &handler)
        {
            m_handleClose = handler;
        }

        void EventFd::SetActive(int events)
        {
            m_activeEvent = events;
        }

        void EventFd::EnableRead(errors::error_code &error)
        {
            m_enableEvent |= EV_READ;
            updateEvent(error);
            enableRead = true;
        }

        void EventFd::EnableWrite(errors::error_code &error)
        {
            m_enableEvent |= EV_WRITE;
            updateEvent(error);
            enableWrite = true;
        }


        int  EventFd::GetEnabledEvent()
        {
            return m_enableEvent;
        }

        bool EventFd::HasBeenEnableRead()
        {
            if (enableRead) {
                return true;
            }
            return false;
        }





        void EventFd::updateEvent(errors::error_code &error)
        {
            m_eventLoop->UpdateEventFd(this, error);
        }

		void EventFd::removeEvent(errors::error_code &error)
		{
			m_eventLoop->RemoveEventFd(this, error);
		}


        EventLoop *EventFd::ItsLoop()
        {
            return m_eventLoop;
        }

        void EventFd::HandleEventWithGuard()
        {
            errors::error_code error;
            if (m_activeEvent & EV_READ &&
                m_handleRead) {
                m_handleRead(error);
            }
            if (m_activeEvent & EV_WRITE &&
                m_handleWrite) {
                m_handleWrite(error);
            }
            if (m_activeEvent & EV_CLOSE &&
                m_handleClose) {
                m_handleClose(error);
            }
            if (m_activeEvent & EV_ERROR &&
                m_handleError) {
                m_handleError(error);
            }
        }
        void EventFd::HandleEvent()
        {
            std::shared_ptr<void> guard;
            if (tied_) {
                guard = tie_.lock();
                if (guard) {
                    HandleEventWithGuard();
                }
            }
            else {
                HandleEventWithGuard();
            }
        }

    }
}
