#include "overlapped_pipe.h"

#include <Windows.h>


namespace pp {
    namespace io {
        HANDLE OverlappedNamedPipe::create(const char *name, errors::error_code &error)
        {
            HANDLE h  = ::CreateNamedPipe(TEXT(name),
                PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE,
                PIPE_UNLIMITED_INSTANCES,  // number of instances
                100 * sizeof(TCHAR), // output buffer size
                100 * sizeof(TCHAR), // input buffer size
                0,            // client time-out
                NULL);
            if (h == INVALID_HANDLE_VALUE)
            {
                error = hht_make_error_code(static_cast<std::errc>(::GetLastError()));
                return NULL;
            }
            
            memset(&ov, 0, sizeof(ov));
            BOOL bRet = ConnectNamedPipe(h, (LPOVERLAPPED)&ov);
            DWORD dwGLE = GetLastError();
            if (!bRet && dwGLE != ERROR_IO_PENDING)
            {
                error = hht_make_error_code(static_cast<std::errc>(dwGLE));
                return NULL;
            }
            return h;
        }

        int OverlappedNamedPipe::Fd()
        {
            return (int)h_;
        }
    }
}