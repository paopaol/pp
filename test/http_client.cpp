#include <io/io_event_loop.h>
#include <net/http/net_http_client.h>
#include <time/_time.h>

using namespace pp;
using namespace std::tr1::placeholders;

static int              complete_numbers   = 0;
static int              concurrent_numbers = 0;
static _time::timer_ref abort_timer;

static void handle_resp(const net::http_response* resp,
                        const errors::error_code& error, io::event_loop* loop)
{
    //abort_timer->cancel();
    //abort_timer = nullptr;

    ++complete_numbers;
    if (error.value() != 0) {
		std::cout << error.message(); //<< std::endl;
    }
    else {
        //std::cout << "status code:" << resp->status_code << std::endl;
        //printf("version:(%d,%d)\n", resp->version.major, resp->version.minor);
        //std::cout << "status:" << resp->status_line << std::endl;
        //std::cout << "headers:" << std::endl;
        //for (auto header : resp->headers) {
        //    std::cout << header.first << " : " << header.second << std::endl;
        //}
        //std::cout << "body:" << std::endl;
        //std::cout << resp->content << std::endl;
		printf("%d\n", complete_numbers);
    }
    if (complete_numbers == concurrent_numbers) {
        loop->quit();
    }
}

int main(int argc, char* argv[])
{
    io::event_loop   loop;
    net::http_client client(&loop);

    std::string url    = argv[1];
    concurrent_numbers = atoi(argv[2]);

    for (int i = 0; i < concurrent_numbers; i++) {
        auto request = client.new_request(
            url, net::http_method::kGet, std::bind(handle_resp, _1, _2, &loop));

        errors::error_code err;
        client.run(request, err);
        if (err.value() != 0) {
            std::cout << err.message() << std::endl;
            return 1;
        }
        //abort_timer =
        //    _time::new_timer(_time::timer::timer_behavior::oneshot,
        //                     _time::Second * 5, [&]() { printf("timeout\n"); });
    }
    loop.exec();
}
