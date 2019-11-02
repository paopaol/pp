#ifndef PP_ERRORS_H
#define PP_ERRORS_H
#include <memory>
#include <string>

namespace pp {
namespace errors {
class Error;

extern const Error nil;

Error New(const std::string &s);
Error New(int code);
class Error {
public:
  Error();

  Error(const std::string &s);
  const char *what() const;
  bool operator==(const Error &e1);
  bool operator!=(const Error &e1);

  friend Error New(const std::string &s);

private:
  typedef std::shared_ptr<std::string> String;
  mutable String e;
};

} // namespace errors
} // namespace pp

#endif