#include <errors.h>

#include <string>

using namespace std;

namespace pp{
    namespace errors{
        Error New(const string &s)
        {
            Error e(s);
            return e;
        }
    }
}