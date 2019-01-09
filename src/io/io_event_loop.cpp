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
        : tid_(system::this_thread::get_id()),
          execing_(false),
          event_poller_(new event_poller),
          exit_(false),
          timer_queue_(std::make_shared<_time::timer_queue>())
#ifdef WIN32
    {
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
    }  // namespace io

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
        assert(!execing_ && "io::event_loop has been stoped!");
        assert(in_created_thread());
        execing_ = true;
        std::vector<Functor> tmp_func_list_;

        while (!exit_) {
            // first, run pending functions
            {
                system::MutexLockGuard _(func_list_mutex_);
                tmp_func_list_ = func_list_;
                func_list_.clear();
            }
            for (auto functor = tmp_func_list_.begin();
                 functor != tmp_func_list_.end(); functor++) {
                if (*functor) {
                    (*functor)();
                }
            }
            int next_timeout =
                static_cast<int>(timer_queue_->handle_timeout_timer());
            errors::error_code error;
            event_poller_->poll(next_timeout, active_ev_fd_list_, error);
#ifndef WIN32
            for (auto ev = active_ev_fd_list_.begin();
                 ev != active_ev_fd_list_.end(); ev++) {
                (*ev)->handle_event();
            }
#endif
            if (error.value() != 0) {
                fprintf(stderr, "%s\n", error.full_message().c_str());
            }
        }
    }

    void event_loop::run_in_loop(const Functor& func)
    {
        // if (in_created_thread()) {
        //     func();
        // }
        // else {
        move_to_loop_thread(func);
        // }
    }

    void event_loop::move_to_loop_thread(const Functor& func)
    {
        {
            system::MutexLockGuard _(func_list_mutex_);
            func_list_.push_back(func);
        }

        // if (!in_created_thread()) {
        wakeup();
        // }
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
    _time::timer_queue_ref event_loop::get_timer_queue()
    {
        return timer_queue_;
    }
}  // namespace io
}  // namespace pp
