#ifndef PP_H
#define PP_H

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace pp {
typedef std::shared_ptr<void> Defer;
typedef std::vector<char> Slice;

#define DISABLE_COPY_CONSTRCT(cls)                                             \
  cls(const cls &);                                                            \
  cls &operator=(const cls &)

} // namespace pp

//#include "errors.h"
//#include "io.h"
//#include "_time.h"
//#include "buffer.h"

#endif
