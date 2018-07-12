#include <iostream>
#include <pp.h>
#include <string>

#ifdef WIN32
#include <Windows.h>
#else
#include <sys/time.h>
#endif

using namespace pp;

int main(int argc, char* argv[])
{
    auto now = _time::Now();
    std::cout << now.String() << std::endl;
    std::cout << _time::Now().Add(_time::Hour * 1).String() << std::endl;
    auto now2 = now;

    if (!now2.Equal(now)) {
        return 1;
    }
    auto old = now;
#ifdef WIN32
    Sleep(1000);
#else
    sleep(1);
#endif
    auto then = _time::Now();
    std::cout << then.String() << std::endl;
    if (!then.After(old)) {
        return 2;
    }

    if (!old.Before(_time::Now())) {
        return 3;
    }

    auto before = _time::Now();
    then        = before;
    then.Add(_time::Second * 99);
    auto d = then.Sub(before);
    if (!(d == _time::Second * 99)) {
        return 4;
    }

    return 0;
}
