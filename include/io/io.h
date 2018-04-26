#ifndef PP_IO_H
#define PP_IO_H


// #include "io.h"
// #include "error.h"
#include <hht.h>

#include <functional>

namespace pp{
    namespace io{
        typedef std::function<size_t (pp::Slice &buf, errors::Error &e)> ReadFunc;
        typedef std::function<size_t (pp::Slice &buf, errors::Error &e)> WriteFunc;

        extern const errors::Error Eof;

        struct Reader{
            ReadFunc  Read;
        };

        struct Writer{
            WriteFunc Write;
        };
    }
}

#endif