#ifndef ERRORS_WINDOWS_H
#define ERRORS_WINDOWS_H

#include <string>

namespace pp {
	namespace errors {
		extern std::string win_errstr(int code);
	}
}

#endif
