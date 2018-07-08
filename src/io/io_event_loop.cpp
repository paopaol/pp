#include <io/io_event_fd.h>
#include <io/io_event_loop.h>
#include <io/io_event_poller.h>

#include <fmt/fmt.h>

#ifdef WIN32
#if !defined(WIN32_LEAN_AND_MEAN) && (_WIN32_WINNT >= 0x0400) \
    && !defined(USING_WIN_PSDK)
#include <windows.h>
#else
#include <Windows.h>
#include <winsock2.h>
#endif
#include "windows/sys_win_thread_local_storage.h"
#include <windows/io_win_iocp_poller.h>
#endif
#include <system/sys_thread.h>

#include <assert.h>
#include <iostream>
#include <memory>

using namespace std;

namespace pp {
namespace io {

    event_loop::event_loop()
        : tid_(system::this_thread::get_id()), execing_(false),
          event_poller_(new event_poller), exit_(false)
#ifdef WIN32
    {
        std::cerr << error.full_message() << std::endl;
        assert(error.value() == 0);
        std::shared_ptr<iocp_poller> poller_ = std::make_shared<iocp_poller>();

#endif

        event_poller_->poll = [poller_](int                 timeoutms,
                                        event_fd_list&      gotEventFds,
                                        errors::error_code& error) {
            return poller_->poll(timeoutms, gotEventFds, error);
        };

        event_poller_->update_event_fd = [poller_](event_fd*           event,
                                                   errors::error_code& error) {
            poller_->update_event_fd(event, error);
        };
        event_poller_->remove_event_fd = [poller_](event_fd*           event,
                                                   errors::error_code& error) {
            poller_->remove_event_fd(event, error);
        };

		event_poller_->wakeup = [poller_]() { poller_->wakeup(); };


        thread_local_storage_init();
        assert(!thread_already_has_loop());
        loopPushToThread(this);
    }

    event_loop::~event_loop()
    {
        loopPopFromThread();
    }

    bool event_loop::in_created_thread()
    {
        return tid_ == system::this_thread::get_id();
    }

    void event_loop::exec()
    {
        assert(!execing_);
        assert(in_created_thread());
        execing_ = true;

        while (!exit_) {
            errors::error_code error;
            event_poller_->poll(INFINITE, active_ev_fd_list_, error);
            for (auto ev = active_ev_fd_list_.begin();
                 ev != active_ev_fd_list_.end(); ev++) {
                (*ev)->handle_event();
            }
            if (error.value() != 0) {
                fprintf(stderr, "%s\n", error.full_message().c_str());
            }
        }
    }

    void event_loop::run_in_loop(const Functor& func)
    {
        if (in_created_thread()) {
            func();
        }
        else {
            move_to_loop_thread(func);
        }
    }

    void event_loop::move_to_loop_thread(const Functor& func)
    {
        {
            system::MutexLockGuard _(func_list_mutex_);
            func_list_.push_back(func);
        }

        if (!in_created_thread()) {
            wakeup();
        }
    }

    void event_loop::update_event_fd(event_fd* event, errors::error_code& error)
    {
        assert(event->its_loop() == this);
        event_poller_->update_event_fd(event, error);
    }

    void event_loop::remove_event_fd(event_fd* event, errors::error_code& error)
    {
        assert(event->its_loop() == this);
        event_poller_->remove_event_fd(event, error);
    }

    void event_loop::wakeup()
    {
		event_poller_->wakeup();

    }

    void event_loop::quit()
    {
        exit_ = true;
        if (!in_created_thread()) {
            wakeup();
        }
    }
}  // namespace io
}  // namespace pp
