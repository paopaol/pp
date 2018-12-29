#include <io/io_event_loop.h>
#include <net/http/net_http_client.h>

using namespace pp;

int main(int argc, char* argv[])
{
    io::event_loop   loop;
    net::http_client client(&loop);
    int              count          = atoi(argv[2]);
    int              total          = 0;
    int              current_number = 0;

    std::string url = argv[1];
    for (int i = 0; i < count; i++) {
        auto request = client.new_request(
            url, net::http_method::kGet,
            [&](const net::http_response* resp,
                const errors::error_code& error) {
                if (error.value() != 0) {
                    std::cout << error.message() << std::endl;
                }
                else {
                    std::cout << "status code:" << resp->status_code
                              << std::endl;
                    printf("version:(%d,%d)\n", resp->version.major,
                           resp->version.minor);
                    std::cout << "status:" << resp->status_line << std::endl;
                    std::cout << "headers:" << std::endl;
                    for (auto header : resp->headers) {
                        std::cout << header.first << " : " << header.second
                                  << std::endl;
                    }
                    std::cout << "body:" << std::endl;
                    std::cout << resp->content << std::endl;
                }
                ++total;
                if (total == count) {
                    loop.quit();
                }
            });

        errors::error_code err;
        client.run(request, err);
        if (err.value() != 0) {
            std::cout << err.message() << std::endl;
            return 1;
        }
    }
    loop.exec();
}
