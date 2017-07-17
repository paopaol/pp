#include <pp.h>
#include <iostream>
#include <string>

using namespace pp;

int main(int argc, char *argv[])
{
    auto now = _time::Now();
    std::cout << now.String() << std::endl;
    std::cout << now.Add(_time::Hour * 1).String() << std::endl;
    auto now2 = now;

    if (!now2.Equal(now)){
        return 1;
    }
    auto old = now;
    _time::Sleep(_time::Second * 1);

    if (!_time::Now().After(old)){
        return 2;
    }

    if (!old.Before(_time::Now())){
        return 3;
    }

    now = _time::Now();
    auto later = now.Add(_time::Second * 99);
    auto d = later.Sub(now);
    if (! (d == _time::Second * 99)){
        return 4;
    }
    



    return 0;
}
