#ifndef OVERPAPPED_FILE_H
#define OVERPAPPED_FILE_H

#include <errors/hht_error.h>
#include <Windows.h>
#include <string>

namespace pp {
    namespace io {
        class OverlappedNamedPipe {
        public:
            OverlappedNamedPipe(const std::string &name)
                :name_(name)
            {
                errors::error_code error;
               h_ = create(name_.c_str(), error);
            }

            ~OverlappedNamedPipe()
            {
                ::CloseHandle(h_);
            }
            int fd();

            std::string Name() {
                return name_;
            }

        private:
            HANDLE create(const char *name, errors::error_code &error);

            OverlappedNamedPipe(const OverlappedNamedPipe&);
            OverlappedNamedPipe &operator=(const OverlappedNamedPipe&);


            HANDLE h_;
            std::string name_;
            OVERLAPPED ov;
        };

    }
}

#endif