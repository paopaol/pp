#include <pp.h>

using namespace pp;

int main(int argc, char *argv[])
{
    errors::Error   e;
    auto b = bytes::NewBuffer();

    b->Write("jinzhao");
    b->Write(" 1988");
    printf("%d\n", b->Len());
    char    name[1024]    = {0};
    b->Read(name, 2);
    b->Optimize();
    if (b->UnReadByte(e) == false){
        printf("%s\n", e.what());
    }
    return 0 ;


}
