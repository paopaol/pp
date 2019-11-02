//
// Created by jz on 17-3-7.
//

#include <stdarg.h>
#include <stdio.h>
#include <string>

#include <fmt/fmt.h>

namespace pp {
namespace fmt {
using namespace std;

string Sprintf(const char *format, ...) {
#define BUFSIZE_MAX 1024 * 50

  char buf[BUFSIZE_MAX] = {0};
  va_list args;

  va_start(args, format);

  vsnprintf(buf, sizeof(buf) - 1, format, args);
  va_end(args);

  string str = buf;

  return str;
}

int Printf(const char *format, ...) {
  va_list args;

  va_start(args, format);
  int n = vfprintf(stdout, format, args);
  va_end(args);
  return n;
}

int Println(const char *format, ...) {
  va_list args;

  va_start(args, format);
  int n = vfprintf(stdout, format, args);
  printf("\n");
  va_end(args);
  return n;
}
} // namespace fmt

} // namespace pp
