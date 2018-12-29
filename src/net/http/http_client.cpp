#include <net/http/net_http_client.h>

#include <http_parser.h>

using namespace std::tr1::placeholders;

namespace pp {
namespace net {
#define __user_data(ptr, type) ptr->user_data().AnyCast<type>()

    class http_conn_ctx {
    public:
        http_conn_ctx(const http_request_ref& request, http_parser_type type)
            : request_(request),
              parser_(std::make_shared<http_parser>()),
              type_(type),
              parse_complete_(false)
        {
            http_parser_init(parser_.get(), type);
            parser_->data = static_cast<void*>(this);
            resp_.request = request_;
        }

        std::string build_request_line();
        std::string build_request_header();
        size_t      build_some_body(char* buffer, int len);

    private:
        friend class http_client;
        friend int        on_message_begin(http_parser* p);
        friend static int on_message_complete(http_parser* _);
        friend int        on_url(http_parser* _, const char* at, size_t length);
        friend int        on_header_field(http_parser* _, const char* at,
                                          size_t length);
        friend int        on_header_value(http_parser* _, const char* at,
                                          size_t length);
        friend int on_body(http_parser* _, const char* at, size_t length);
        friend int on_status(http_parser* _, const char* at, size_t length);
        void       parsing_http_msg(bytes::Buffer& buffer);
        int        parse_url();

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

    const static char* kUserAgent =
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, "
        "like Gecko) Chrome/70.0.3538.102 Safari/537.36 OPR/57.0.3098.106";

    const static char* kCacheControl = "max-age=0";

    const static char* kAccept = "*/*";
    // const static char* kAccept =
    //     "text/html,application/xhtml+xml,application/xml;q=0.9,image/"
    //     "webp,image/apng,*/*;q=0.8";
    const static char* kAcceptLanguage = "zh-CN,zh;q=0.9,en-US;q=0.8,en;q=0.7";

    static int on_message_begin(http_parser* p)
    {
        return 0;
    }

    static int on_headers_complete(http_parser* p)
    {
        return 0;
    }
    static int on_message_complete(http_parser* _)
    {
        auto ctx = static_cast<http_conn_ctx*>(_->data);
        assert(ctx);
        ctx->parse_complete_ = true;
        return 0;
    }
    int on_url(http_parser* _, const char* at, size_t length)
    {
        auto ctx = static_cast<http_conn_ctx*>(_->data);
        assert(ctx);

        switch (ctx->type_) {
        case HTTP_RESPONSE: {
            // no url, do nothing
            break;
        }
        default:
            break;
        }
        return 0;
    }
    int on_header_field(http_parser* _, const char* at, size_t length)
    {
        auto ctx = static_cast<http_conn_ctx*>(_->data);
        assert(ctx);

        ctx->next_header_ = std::string(at, length);
        return 0;
    }

    int on_header_value(http_parser* _, const char* at, size_t length)
    {
        auto ctx = static_cast<http_conn_ctx*>(_->data);
        assert(ctx);
        switch (ctx->type_) {
        case HTTP_RESPONSE: {
            ctx->resp_.headers[ctx->next_header_] = std::string(at, length);
            break;
        }
        default:
            break;
        }
        return 0;
    }

    int on_body(http_parser* _, const char* at, size_t length)
    {
        auto ctx = static_cast<http_conn_ctx*>(_->data);
        assert(ctx);

        switch (ctx->type_) {
        case HTTP_RESPONSE: {
            if (ctx->resp_.body.is_nil()) {
                ctx->resp_.content.append(at, length);
            }
            else {
                ctx->resp_.body.write(at, length);
            }
            break;
        }
        default:
            break;
        }
        return 0;
    }

    int on_status(http_parser* _, const char* at, size_t length)
    {
        auto ctx = static_cast<http_conn_ctx*>(_->data);
        assert(ctx && ctx->type_ == HTTP_RESPONSE);

        ctx->resp_.status_line = std::string(at, length);
        return 0;
    }

    std::string http_conn_ctx::build_request_line()
    {
        http_method method = request_->method;
        std::string version =
            request_->version == http_version::kV1_0 ? "HTTP/1.0" : "HTTP/1.1";
        int         off = parse_url_.field_data[UF_PATH].off;
        int         len = parse_url_.field_data[UF_PATH].len;
        std::string path(request_->url.c_str() + off, len);

        switch (method) {
        case http_method::kGet: {
            return std::string("GET ") + path + " " + version;
            break;
        }
        default:
            break;
        }
        return "";
    }

    static http_parser_settings parser_settings = {
        on_message_begin, on_url,
        on_status,        on_header_field,
        on_header_value,  on_headers_complete,
        on_body,          on_message_complete
    };

    void http_conn_ctx::parsing_http_msg(bytes::Buffer& buffer)
    {
        // if already had parser error before
        // return
        if (error_.value() != 0) {
            return;
        }

        char*  data = nullptr;
        size_t size = buffer.Len();
        size        = buffer.ZeroCopyRead(data, buffer.Len());

        int nparsed =
            http_parser_execute(parser_.get(), &parser_settings, data, size);
        if (nparsed != size) {
            // if there some error, we store the error code,
            // and call ther user callback when the tcp conn closed
            const char* str =
                http_errno_name(static_cast<http_errno>(parser_->http_errno));
            error_ = hht_make_error_code(
                static_cast<errors::error>(errors::error::NET_ERROR));
            error_.suffix_msg(str);
        }
    }

    int http_conn_ctx::parse_url()
    {
        http_parser_url_init(&parse_url_);
        int ret = http_parser_parse_url(
            request_->url.c_str(), request_->url.size(), false, &parse_url_);
        if (ret != 0) {
            return -1;
        }
        return 0;
    }

    std::string http_conn_ctx::build_request_header()
    {
        std::string longline;

        for (auto header = request_->headers.begin();
             header != request_->headers.end(); header++) {
            longline += header->first;
            longline += ":";
            longline += header->second;
            longline += "\r\n";
        }
        longline += "\r\n";
        return longline;
    }

    size_t http_conn_ctx::build_some_body(char* buffer, int len)
    {
        return request_->body.read(buffer, len);
    }

    http_client::http_client(io::event_loop* loop) : loop_(loop) {}

    http_request_ref
    http_client::new_request(const std::string& url, http_method method,
                             const http_response_handler& resp_handler)
    {
        auto request           = std::make_shared<http_request>();
        request->method        = method;
        request->url           = url;
        request->version       = http_version::kV1_1;
        request->resp_handler_ = resp_handler;
        // request->headers["Host"]            = "220.181.112.244";
        request->headers["Cache-Control"]   = kCacheControl;
        request->headers["Accept"]          = kAccept;
        request->headers["Accept-Language"] = kAcceptLanguage;
        request->headers["User-Agent"]      = kUserAgent;

        return request;
    }

    void http_client::write_request_line(const tcp_conn_wref&      conn,
                                         const http_conn_ctx_wref& ctx)
    {
        auto rconn = conn.lock();
        auto rctx  = ctx.lock();

        assert(rconn && rctx);

        // next send header
        rconn->data_write_finished(
            std::bind(&http_client::write_headers, this, conn, ctx));

        auto request_line = rctx->build_request_line();
        rconn->write(request_line.c_str(), request_line.size());
        rconn->write("\r\n", 2);
    }

    void http_client::write_headers(const tcp_conn_wref&      conn,
                                    const http_conn_ctx_wref& ctx)
    {
        auto rconn = conn.lock();
        auto rctx  = ctx.lock();

        assert(rconn && rctx);

        rconn->data_write_finished(
            std::bind(&http_client::write_body, this, conn, ctx));
        auto header_line = rctx->build_request_header();
        rconn->write(header_line.c_str(), header_line.size());
        rconn->async_read();
    }

    void http_client::write_body(const tcp_conn_wref&      conn,
                                 const http_conn_ctx_wref& ctx)
    {
        auto rconn = conn.lock();
        auto rctx  = ctx.lock();

        assert(rconn && rctx);

        char some[4096] = { 0 };
        int  len        = ( int )rctx->build_some_body(some, sizeof some);
        if (len == 0) {
            rconn->data_write_finished(nullptr);
        }
        rconn->write(some, len);
    }

    void http_client::remove_client(const http_conn_ctx_ref& ctx)
    {
        assert(ctx);
        auto found = client_list_.find(ctx);
        assert(found != client_list_.end());
        client_list_.erase(found);
    }

    void http_client::add_client(const tcp_client_ref&    client,
                                 const http_conn_ctx_ref& ctx)
    {
        client_list_[ctx] = client;
    }

    void http_client::handle_resp(const http_response*      resp,
                                  const errors::error_code& error)
    {
        auto request = resp->request.lock();

        if (request && request->resp_handler_) {
            request->resp_handler_(resp, error);
        }
    }

    void http_client::run(const http_request_ref& request)
    {
        assert(request);

        auto ctx = std::make_shared<http_conn_ctx>(request, HTTP_RESPONSE);

        int ret = ctx->parse_url();
        if (ret != 0) {
            // bad url
            return;
        }

        int         off  = ctx->parse_url_.field_data[UF_HOST].off;
        int         len  = ctx->parse_url_.field_data[UF_HOST].len;
        int         port = ctx->parse_url_.port;
        std::string host = std::string(ctx->request_->url.c_str() + off, len);

        auto client = std::make_shared<tcp_client>(loop_, addr(host, port));
        ctx->request_->headers["Host"] = host;

        client->dial_done(
            std::bind(&http_client::send_request, this, _1, _2, _3));
        client->message_recved(
            std::bind(&http_client::recv_resp, this, _1, _2, _3));

        client->set_user_data(http_conn_ctx_wref(ctx));

        client->dial();
        add_client(client, ctx);
    }

    void http_client::send_request(const net::tcp_conn_ref&  conn,
                                   const _time::time&        now,
                                   const errors::error_code& error)
    {
        auto ctx = __user_data(conn, http_conn_ctx_wref).lock();
        assert(ctx);

        auto request = ctx->request_;
        auto resp    = ctx->resp_;

        if (!conn->connected() || error.value() != 0) {
            auto err = ctx->error_.value() == 0 ? error : ctx->error_;
            handle_resp(&ctx->resp_, err);
            // conn closed
            remove_client(ctx);
            return;
        }
        write_request_line(conn, ctx);
    }

    void http_client::recv_resp(const tcp_conn_ref& conn, bytes::Buffer& buffer,
                                const _time::time& time)
    {
        auto ctx = __user_data(conn, http_conn_ctx_wref).lock();
        assert(ctx);

        ctx->parsing_http_msg(buffer);
        conn->async_read();
        if (ctx->parse_complete_) {
            conn->shutdown();
        }
    }
}  // namespace net
}  // namespace pp
