#ifndef EVENT_LOOP_H
#define EVENT_LOOP_H

#include <hht.h>

#include <system/thread.h>
#include <system/mutex.h>
#include <vector>

#ifdef WIN32
#include <windows/overlapped_pipe.h>
#endif
#include <errors/hht_error.h>

namespace pp {
    namespace io {
		typedef std::function<void()> Functor;

        class EventFd;
        class EventPoller;
        typedef std::vector<EventFd *> EventFdList;
        class EventLoop {
        public:
            EventLoop();
            ~EventLoop();
            void Exec();
            void Wakeup();
            void Quit();
			void RunInLoop(const Functor &func);

            void UpdateEventFd(EventFd *event, errors::error_code &error);
			void RemoveEventFd(EventFd *event, errors::error_code &error);
            bool InCreateThread();

        private:
            bool threadAlreadyExistLoop();
			void moveToLoopThread(const Functor &func);

			DISABLE_COPY_CONSTRCT(EventLoop);



            system::thread_id						m_tid;
            bool 			                        m_execing;
            bool 			                        exit_;
            std::shared_ptr<EventPoller>            eventPoller;
            errors::error_code                      error;
            EventFdList  	                        m_activeEvents;
            OverlappedNamedPipe                     wakeupPipe_;
            std::shared_ptr<EventFd>                wakeupEventFd_;
			std::vector<Functor>					functorList_;
			system::Mutex							functorListMutex_;
        };

    }
}

#endif // EVENT_LOOP_H
