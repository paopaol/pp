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



		static shared_ptr<string> e_nil = make_shared<string>("nil");
		const Error               nil;

		Error::Error()
		{
			e = e_nil;
		}

		Error::Error(const std::string& s) : e(std::make_shared<std::string>(s)) {}
		const char* Error::what() const
		{
			return e->c_str();
		}

		bool Error::operator==(const Error& e1)
		{
			return e1.e == e;
		}

		bool Error::operator!=(const Error& e1)
		{
			return e1.e != e;
		}

		Error New(const string& s)
		{
			Error e(s);
			return e;
		}
	}  // namespace errors
}  // namespace pp

namespace std {

std::error_code make_error_code(pp::errors::error e)
{
	return
	{
		static_cast<int>(e), pp::errors::pp_error_category_
	};
}
}

