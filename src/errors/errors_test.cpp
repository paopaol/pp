#include <pp.h>
#include <string.h>
#include <tuple>

using namespace pp;

static void test_New()
{
    auto e = errors::New("new error");
    printf("the errors is %s\n", e.what());

    return;
}

static bool test_error_eq()
{
    auto e = io::Eof;

    return (e == io::Eof ? true : false);
}

static void test_GetError(errors::Error &e)
{
    e = io::Eof;
    
    return;
}

static auto myError = errors::Error("it's my error");


static std::tuple<int  , errors::Error > test_return_tuple()
{
  return std::make_tuple(-1, myError);
}



int main(int argc, char *argv[])
{
    if (argc != 2){
        return 1;
    }
    if (strcmp(argv[1], "new") == 0){
        test_New();
    }else if (strcmp(argv[1], "get") == 0){
        errors::Error e;
        test_GetError(e);
        printf("the errors is %s\n", e.what());
    }else if (strcmp(argv[1], "eq") == 0){
        printf("%d\n", test_error_eq());
    }else if (strcmp(argv[1], "return_tuple") == 0){
      int           n;
      errors::Error e;
      std::tie(n, e) = test_return_tuple();
      printf("%d %s\n", n, e.what());
    }


    return 0;
}
