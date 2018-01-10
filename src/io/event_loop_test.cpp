#include <event_loop.h>

#include <thread>

using namespace std;
using namespace pp;


void other_worker()
{
        io::EventLoop  app2;
        app2.Exec();
}


int main(int argc, char *argv[])
{
    io::EventLoop app;
    thread t(other_worker);
    t.detach();

    app.Exec();
}
