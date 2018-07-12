#ifndef OVERPAPPED_FILE_H
#define OVERPAPPED_FILE_H

#include <errors/hht_error.h>
#include <io/io_event_fd.h>
#include <string>

#include <Windows.h>


namespace pp {
namespace io {
    typedef std::function<void(int bytes_of_write, const errors::error_code& error)>
        write_done_handler;
    typedef std::function<void(const bytes::Buffer& buffer,
                               const errors::error_code& error)>
        read_done_handler;
    typedef std::function<void(const errors::error_code& error)> close_done_handler;
    class event_loop;
    class pipeline {
    public:
        pipeline(event_loop* loop);
        ~pipeline();
        int  fd();
        void write(const char* data, int len,
                   const write_done_handler& on_write);
        void read(const read_done_handler& on_read);
        void close(const close_done_handler& on_close);

    private:
        pipeline(const pipeline&);
        pipeline& operator=(const pipeline&);

        errors::error_code error_;
        HANDLE             h_;
        std::string        name_;
        OVERLAPPED         ov;
        event_loop*        loop_;
        event_fd_ref       ev_;
        write_done_handler on_write_;
        read_done_handler  on_read_;
		close_done_handler on_close_;
    };
}
}

#endif
