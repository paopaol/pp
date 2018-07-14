#include <hht.h>

using namespace pp;

int main(int argc, char *argv[])
{
    errors::Error   e;
    bytes::Buffer b;

    b.Write("jinzhao");
    b.Write(" 1988");
    printf("%d\n", b.Len());
    char    name[1024] = {0};
    b.Read(name, 2);

    return 0 ;


}
