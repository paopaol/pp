#ifndef PP_ERROR_H
#define PP_ERROR_H

#include <system_error>

namespace pp {
namespace errors {
enum class error { PP_NO_ERROR, NET_ERROR };
class pp_error_category : public std::error_category {
public:
  const char *name() const noexcept;

  std::string message(int code) const;
};
} // namespace errors
} // namespace pp

namespace std {
template <> struct is_error_code_enum<pp::errors::error> : true_type {};

} // namespace std

namespace std {
std::error_code make_error_code(pp::errors::error e);
}

#endif
