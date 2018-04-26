#include <errors.h>
#include <string>
#include <stdio.h>

using namespace pp;
using namespace std;

int main()
{
	auto Timeout = errors::New("time out");
	errors::Error e("");

	if (e == errors::nil) {
		printf("true\n");
	}

	e = Timeout;
	if (e == Timeout) {
		printf("true\n");
	}
}