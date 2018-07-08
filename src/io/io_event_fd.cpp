#include <io/io_event_fd.h>

#include <io/io_event_loop.h>

namespace pp {
namespace io {
    event_fd::event_fd(event_loop* loop, int fd)
        : event_loop_(loop), fd_(fd), enabled_event_(EV_NONE),
          active_event_(EV_NONE), tied_(false)
    {
    }

    void event_fd::set_write_handler(const event_handler& handler)
    {
        handler_write_ = handler;
    }

    void event_fd::data_recved(const event_handler& handler)
    {
        handler_read_ = handler;
    }

    void event_fd::set_error_handler(const event_handler& handler)
    {

        handler_error_ = handler;
    }

    void event_fd::disconnected(const event_handler& handler)
    {
        handler_close_ = handler;
    }

    void event_fd::set_active(int events)
    {
        active_event_ = events;
    }

    void event_fd::enable_read(errors::error_code& error)
    {
        enabled_event_ |= EV_READ;
        update_event(error);
    }

    void event_fd::enable_write(errors::error_code& error)
    {
        enabled_event_ |= EV_WRITE;
        update_event(error);
    }

    int event_fd::enabled_event()
    {
        return enabled_event_;
    }

    void event_fd::update_event(errors::error_code& error)
    {
        event_loop_->update_event_fd(this, error);
    }

    void event_fd::remove_event(errors::error_code& error)
    {
        event_loop_->remove_event_fd(this, error);
    }

    event_loop* event_fd::its_loop()
    {
        return event_loop_;
    }

    void event_fd::handle_event_with_guard()
    {
        errors::error_code error;
        if (active_event_ & EV_READ && handler_read_) {
            handler_read_(error);
        }
        if (active_event_ & EV_WRITE && handler_write_) {
            handler_write_(error);
        }
        if (active_event_ & EV_CLOSE && handler_close_) {
            handler_close_(error);
        }
        if (active_event_ & EV_ERROR && handler_error_) {
            handler_error_(error);
        }
    }
    void event_fd::handle_event()
    {
        std::shared_ptr<void> guard;
        if (tied_) {
            guard = tie_.lock();
            if (guard) {
                handle_event_with_guard();
            }
        }
        else {
            handle_event_with_guard();
        }
    }
}
}
