#ifndef HTTP_CONN_CTX_H
#define HTTP_CONN_CTX_H


#include <net/http/net_http_client.h>
#include <http_parser.h>

namespace pp {
namespace net {
    class http_conn_ctx {
	public:
        http_conn_ctx(const http_request_ref& request, http_parser_type type);
    private:
        std::string build_request_line();
        std::string build_request_header();
        size_t      build_some_body(char* buffer, int len);
        void        parsing_http_msg(bytes::Buffer& buffer);
        int         parse_url();

        friend class http_client;
        friend int        on_message_begin(http_parser* p);
        friend static int on_headers_complete(http_parser* p);
        friend static int on_message_complete(http_parser* _);
        friend static int on_url(http_parser* _, const char* at, size_t length);
        friend static int on_header_field(http_parser* _, const char* at,
                                          size_t length);
        friend static int on_header_value(http_parser* _, const char* at,
                                          size_t length);
        friend static int on_body(http_parser* _, const char* at,
                                  size_t length);
        friend static int on_status(http_parser* _, const char* at,
                                    size_t length);

        std::string                  next_header_;
        std::string                  next_val_;
        http_request_ref             request_;
        http_response                resp_;
        std::shared_ptr<http_parser> parser_;
        http_parser_type             type_;
        errors::error_code           error_;
        bool                         parse_complete_;
        http_parser_url              parse_url_;
    };
}  // namespace net
}  // namespace pp
#endif /* HTTP_CONN_CTX_H */
