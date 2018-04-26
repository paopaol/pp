#include <errors.h>

#include <string>

using namespace std;

namespace pp{
	namespace errors {
		static shared_ptr<string> e_nil = make_shared<string>("nil");
		const Error nil;

		Error::Error() {
			e = e_nil;
		}

		Error::Error(const std::string &s)
			:e(std::make_shared<std::string>(s))
		{
		}
		const char *Error::what() const
		{
			return e->c_str();
		}

		bool Error::operator == (const Error &e1)
		{
			return e1.e == e;
		}

		bool Error::operator != (const Error &e1)
		{
			return e1.e != e;
		}

		Error New(const string &s)
		{
			Error e(s);
			return e;
		}
	}
}