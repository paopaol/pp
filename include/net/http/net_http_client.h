#ifndef NET_HTTP_CLIENT_H
#define NET_HTTP_CLIENT_H

#include <errors/hht_error.h>
#include <functional>
#include <map>
#include <memory>

#include <bytes/buffer.h>
#include <io/io.h>
#include <net/net_tcp_client.h>

namespace pp {
class io::event_loop;
}

namespace pp {
namespace net {
    enum class http_method { kGet, kPost };
    enum class http_version { kV1_1, kV1_0 };

    typedef std::map<std::string, std::string> http_header;

    class http_response;
    typedef std::shared_ptr<http_response> http_response_ref;
    typedef std::function<void(const http_response*      resp,
                               const errors::error_code& error)>
        http_response_handler;

    class http_request {
    public:
        http_request(){};

        friend class http_client;

        http_method  method;
        std::string  url;
        http_version version;
        http_header  headers;

        // when body is nil,we use content
        std::string content;
        io::reader  body;

    private:
        http_response_handler resp_handler_;
    };

    typedef std::shared_ptr<http_request> http_request_ref;
    typedef std::weak_ptr<http_request>   http_request_wref;

    class http_conn_ctx;
    typedef std::shared_ptr<http_conn_ctx> http_conn_ctx_ref;

    class http_response {
    public:
        int          status_code;
        std::string  status_line;
        http_header  headers;
        http_version version;
        // when body is nil,we use content
        std::string                 content;
        io::writer                  body;
        std::weak_ptr<http_request> request;
    };

    class http_client {
    public:
        //! Default constructor
        http_client(io::event_loop* loop);
        ~http_client(){};
        http_request_ref new_request(const std::string& url, http_method method,
                                     const http_response_handler& resp_handler);
        void             run(const http_request_ref& request);

    private:
        typedef std::weak_ptr<http_conn_ctx> http_conn_ctx_wref;
        typedef std::weak_ptr<tcp_conn>      tcp_conn_wref;

        //! Copy constructor
        http_client(const http_client& other);

        //! Move constructor
        http_client(http_client&& other) noexcept;

        //! Copy assignment operator
        http_client& operator=(const http_client& other);

        //! Move assignment operator
        http_client& operator=(http_client&& other) noexcept;

        void write_request_line(const tcp_conn_wref&      conn,
                                const http_conn_ctx_wref& ctx);
        void write_headers(const tcp_conn_wref&      conn,
                           const http_conn_ctx_wref& ctx);
        void write_body(const tcp_conn_wref&      conn,
                        const http_conn_ctx_wref& ctx);
        void remove_client(const http_conn_ctx_ref& client);
        void add_client(const tcp_client_ref&    client,
                        const http_conn_ctx_ref& ctx);

        void send_request(const net::tcp_conn_ref& conn, const _time::time& now,
                          const errors::error_code& error);
        void handle_resp(const http_response*      resp,
                         const errors::error_code& error);

        void recv_resp(const tcp_conn_ref& conn, bytes::Buffer& buffer,
                       const _time::time& time);

        typedef std::map<http_conn_ctx_ref, tcp_client_ref> http_conn_list;

        io::event_loop* loop_;
        http_conn_list  client_list_;
    };
}  // namespace net
}  // namespace pp

#if 0
int main()
{
    io::event_loop loop;
    http_client    client(&loop);

    auto request = client.new_request(
        "GET", "http://www.baidu.com",
        [&, std::weak_ptr<http_request>(request)](
            const net::http_response_ref& resp,
            const errors::error_code&     error) {
            if (error.value() != 0 || !conn->connected()) {
                return;
            }
        },
        [&](const net::http_response_ref& resp, const bytes::buffer& buffer,
            const errors::error_code& error) {
            std::vector<char> data;
            buffer.read(data);
        });
    request->header.set("xxxxx", "xxxxxx");
    client.do(request);


    request = client.new_request(
        "POST", "http://www.baidu.com",
        [&](http_response* resp, const errors::error_code& error) {},
        [&](http_response* resp, const bytes::buffer& buffer,
            const errors::error_code& error) {

        });

    request.body    = io::reader([](char* buffer, int len) -> int {
        std::copy(buffer, "key=value", len);
        return 9;
    });
    request.readall = true;  // defult is true
    client.do(request);
    // if conn is closed, request destructed
    loop.exec();
}
#endif

#endif /* NET_HTTP_CLIENT_H */
