#ifndef PP_STRINGS_H
#define PP_STRINGS_H

#include <string>
#include <vector>

namespace pp {
namespace strings {
std::string TrimSpace(const std::string &str);
std::string ToUpper(const std::string &str);
std::string ToLower(const std::string &str);
std::vector<std::string> &Split(std::vector<std::string> &vstr,
                                const std::string &str,
                                const std::string &pattern);
bool Contains(const std::string &s, const std::string &substr);
std::string Join(const std::vector<std::string> &a, const std::string &sep);
} // namespace strings
} // namespace pp

#endif
