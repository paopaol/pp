#ifndef PP_IO_H
#define PP_IO_H

// #include "io.h"
// #include "error.h"
#include <hht.h>

#include <functional>

namespace pp {
namespace io {
    // read_func and write_func should never return error,
    // if there is no data for read or write,return 0(EOF)
    typedef std::function<size_t(char* buf, size_t len)>       read_func;
    typedef std::function<size_t(const char* buf, size_t len)> write_func;

    class reader {
    public:
        explicit reader(const read_func& func) : read_(func) {}
        reader(){};
        ~reader(){};

        size_t read(char* buf, size_t len)
        {
            if (read_) {
                return read_(buf, len);
            }
            return 0;
        }
        bool is_nil()
        {
            return read_ == nullptr;
        }

    private:
        read_func read_;
    };

    class writer {
    public:
        writer(const write_func& func) : write_(func){};
        writer(){};
        ~writer(){};

        size_t write(const char* buf, size_t len)
        {
            if (write_) {
                return write_(buf, len);
            }
            return 0;
        }
        bool is_nil()
        {
            return write_ == nullptr;
        }

    private:
        write_func write_;
    };

}  // namespace io
}  // namespace pp

#endif
