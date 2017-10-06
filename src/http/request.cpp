#include <cstdcall.h>
#include <http_client.h>



namespace cstdcall{
	using namespace std;
	namespace http{
		Request::Request(const string &method, const string &url)
			:Method(method),
			Url(url),
			ContentLength(0)
		{
		}
	}
}
