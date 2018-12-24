#include <errors.h>
#include <errors/pp_error.h>

#include <string>

using namespace std;

namespace pp {
namespace errors {

    const char* pp_error_category::name() const noexcept
    {
        return "pp::pp_error";
    }

    std::string pp_error_category::message(int code) const
    {
        switch (static_cast<error>(code)) {
        case error::PP_NO_ERROR:
            return "no error";
        case error::NET_ERROR:
            return "net error";
        default:
            return "unkown error";
        }
    }
    static const pp_error_category pp_error_category_{};
}  // namespace errors
}  // namespace pp

namespace std {

std::error_code make_error_code(pp::errors::error e)
{
    return { static_cast<int>(e), pp::errors::pp_error_category_ };
}
}  // namespace std
