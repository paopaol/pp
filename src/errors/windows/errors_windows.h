#ifndef ERRORS_WINDOWS_H
#define ERRORS_WINDOWS_H

#include <string>

namespace pp {
	namespace errors {
		extern std::string windows_errstr(int code);
	}
}

#endif
