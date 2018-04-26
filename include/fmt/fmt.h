#ifndef PP_FMT_H
#define PP_FMT_H

#include <string>

namespace pp{

    namespace fmt {
        std::string Sprintf(const char *format, ...);
        int Printf(const char *format, ...);
        int Println(const char *format, ...);
    }
}

#endif
