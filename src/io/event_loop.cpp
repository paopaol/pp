#include <io/event_loop.h>
#include <fmt/fmt.h>

#include "thread_local_storage.h"
#include <io/event_fd.h>
#include <io/event_poller.h>
#ifdef WIN32
#include <windows/event_iocp_poller.h>
#endif
#include <system/thread.h>


#include <assert.h>
#include <iostream>
#include <memory>

using namespace std;

namespace pp {
    namespace io {

        EventLoop::EventLoop()
            :m_tid(system::this_thread::get_id())
            ,m_execing(false)
            ,eventPoller(new EventPoller)
            ,exit_(false)
#ifdef WIN32
            ,wakeupPipe_(fmt::Sprintf("\\\\.\\pipe\\wakeuppipe%d", m_tid).c_str())
            ,wakeupEventFd_(new IocpEventFd(this, wakeupPipe_.Fd()))
        {
            std::cerr << error.full_message() << std::endl;
            assert(error.value() == 0);
            std::shared_ptr<EventIocpPoller> Poller_ = std::make_shared<EventIocpPoller>();
            
#endif

            eventPoller->Poll = [Poller_](int timeoutms,
                EventFdList &gotEventFds, errors::error_code &error) {
                return  Poller_->Poll(timeoutms, gotEventFds, error);
            };

            eventPoller->UpdateEventFd = [Poller_](EventFd *event, errors::error_code &error) {
                Poller_->UpdateEventFd(event, error);
            };
            eventPoller->RemoveEventFd = [Poller_](EventFd *event, errors::error_code &error) {
                Poller_->RemoveEventFd(event, error);
            };
#ifdef WIN32
            IocpEventFd *event = static_cast<IocpEventFd *>(wakeupEventFd_.get());
            event->EnableWakeUp(error);
#endif

            thread_local_storage_init();
            assert(!threadAlreadyExistLoop());
            loopPushToThread(this);
        }

        EventLoop::~EventLoop()
        {
            loopPopFromThread();
        }

        bool EventLoop::InCreateThread()
        {
            return m_tid == system::this_thread::get_id();
        }

        void EventLoop::Exec()
        {
            assert(!m_execing);
            assert(InCreateThread());
            m_execing = true;
            

            while (!exit_) {
                errors::error_code error;
                eventPoller->Poll(INFINITE, m_activeEvents, error);
                for (auto ev = m_activeEvents.begin();
                    ev != m_activeEvents.end(); ev++) {
                    (*ev)->HandleEvent();
                }
                if (error.value() != 0) {
                    fprintf(stderr, "%s\n", error.full_message().c_str());
                }
            }
        }


		void EventLoop::RunInLoop(const Functor &func)
		{
			if (InCreateThread()){
				func();
			}else{
				moveToLoopThread(func);
			}
		}

		void EventLoop::moveToLoopThread(const Functor &func)
		{
			{
				system::MutexLockGuard _(functorListMutex_);
				functorList_.push_back(func);
			}

			if (!InCreateThread()){
				Wakeup();
			}
		}

        void EventLoop::UpdateEventFd(EventFd *event, errors::error_code &error)
        {
            assert(event->ItsLoop() == this);
            eventPoller->UpdateEventFd(event, error);
        }

		void EventLoop::RemoveEventFd(EventFd *event, errors::error_code &error)
		{
			assert(event->ItsLoop() == this);
			eventPoller->RemoveEventFd(event, error);
		}

#ifdef WIN32
        void EventLoop::Wakeup() 
        {
           

            HANDLE h = ::CreateFile(wakeupPipe_.Name().c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL,
                OPEN_EXISTING, 0, NULL);
      
            ::CloseHandle(h);
        }
#endif

        void EventLoop::Quit()
        {
            exit_ = true;
            if (!InCreateThread()) {
                Wakeup();
            }
        }

    }
}
