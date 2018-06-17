#include "io_win_overlapped_pipe.h"

#include "windows/io_win_iocp_event_fd.h"

#include <Windows.h>

namespace pp {
namespace io {
    pipeline::pipeline(event_loop* loop)
        : loop_(loop),
          name_(fmt::Sprintf("\\\\.\\pipe\\pipe_%s"), uuid::u16().string()),
          h_(create(name.c_str(), error_)),
    {
        assert(h_ != nullptr);

        ev_                  = std::make_shared_ptr<iocp_event_fd>(loop_, h_);
        iocp_event_fd* event = static_cast<iocp_event_fd*>(ev_.get());
        event->enbale_read(errror_);
        assert(error_.value() != 0);
    }

    pipeline::~pipeline()
    {

        if (h_) {
            ::CloseHandle(h_);
        }
        if (on_close_) {
            on_close_(std::make_error_code());
        }
    }

    int pipeline::fd()
    {
        return ( int )h_;
    }

    static HANDLE create(const char* name, errors::error_code& error)
    {
        HANDLE h = ::CreateNamedPipe(
            TEXT(name), PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE,
            PIPE_UNLIMITED_INSTANCES,  // number of instances
            65535,       // output buffer size
            65535,       // input buffer size
            0,                         // client time-out
            NULL);
        if (h == INVALID_HANDLE_VALUE) {
            error =
                hht_make_error_code(static_cast<std::errc>(::GetLastError()));
            return NULL;
        }

        memset(&ov, 0, sizeof(ov));
        BOOL  bRet  = ConnectNamedPipe(h, ( LPOVERLAPPED )&ov);
        DWORD dwGLE = GetLastError();
        if (!bRet && dwGLE != ERROR_IO_PENDING) {
            error = hht_make_error_code(static_cast<std::errc>(dwGLE));
            return NULL;
        }
        return h;
    }
}
}
