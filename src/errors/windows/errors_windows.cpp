#include <errors.h>

#include <Windows.h>
#include <fmt/fmt.h>
#include <functional>
#include <windows/errors_windows.h>

namespace pp {
namespace errors {

std::string win_errstr(int code) {
  LPVOID msg;
  FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                    FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL, code, 0 /*Default language*/, (LPTSTR)&msg, 0, NULL);
  if (msg == NULL) {
    return fmt::Sprintf("%d %s", code, "unkown error");
  }
  std::shared_ptr<void> __(nullptr, std::bind(LocalFree, msg));
  return fmt::Sprintf("%d %s", code, msg);
}
} // namespace errors
} // namespace pp
