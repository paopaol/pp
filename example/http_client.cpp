#include <io/io_event_loop.h>
#include <net/http/net_http_client.h>
#include <time/_time.h>

using namespace pp;
using namespace std::tr1::placeholders;

io::event_loop                          loop;
net::http_client                        client(&loop);
static std::weak_ptr<net::http_request> wreq;
static bool                             canceling = false;

static size_t read_body(const char* data, size_t len)
{
    fwrite(data, 1, len, stdout);

    if (canceling) {
        return len;
    }

    auto req = wreq.lock();
    if (!req) {
        return 0;
    }
    client.cancel(req);
    canceling = true;

    return len;
}

// this function will called multi times;
// 1, on header recved
// 2, on errror
// 3, on  resp done

// 1- failed:{
// connect failed
// conn broken close
//}
// 2- success:{
// got resp
//}

static void handle_resp(net::http_response* resp, bool finished,
                        const errors::error_code& error)
{
    if (finished || error) {
        if (error) {
            std::cout << error.message() << std::endl;
        }
        loop.quit();
        return;
    }
    std::cout << "status code:" << resp->status_code << std::endl;
    printf("version:(%d,%d)\n", resp->version.major, resp->version.minor);
    std::cout << "status:" << resp->status_line << std::endl;
    std::cout << "headers:" << std::endl;
    for (auto header : resp->headers) {
        std::cout << header.first << " : " << header.second << std::endl;
    }
    // if set body writer,the body will be readed
    resp->body = io::writer(read_body);
}

int main(int argc, char* argv[])
{

    std::string url = argv[1];

    auto request = client.new_request(url, net::http_method::kGet,
                                      std::bind(handle_resp, _1, _2, _3));

    errors::error_code err;
    client.run(request, err);
    if (err.value() != 0) {
        std::cout << err.message() << std::endl;
        return 1;
    }
    wreq = request;
    loop.exec();
}
