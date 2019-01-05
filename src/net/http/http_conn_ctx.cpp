#include "http_conn_ctx.h"
#include <fmt/fmt.h>

namespace pp {
namespace net {

    http_conn_ctx::http_conn_ctx(const http_request_ref& request,
                                 http_parser_type        type)
        : request_(request),
          parser_(std::make_shared<http_parser>()),
          type_(type),
          parse_complete_(false),
          parse_header_complete_(false)

    {
        http_parser_init(parser_.get(), type);
        parser_->data = static_cast<void*>(this);
        resp_.request = request_;
    }

    const static char* kUserAgent =
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, "
        "like Gecko) Chrome/70.0.3538.102 Safari/537.36 OPR/57.0.3098.106";

    const static char* kCacheControl = "max-age=0";

    const static char* kAccept         = "*/*";
    const static char* kAcceptLanguage = "zh-CN,zh;q=0.9,en-US;q=0.8,en;q=0.7";

    static int on_message_begin(http_parser* p)
    {
        return 0;
    }

    static int on_headers_complete(http_parser* p)
    {
        auto ctx = static_cast<http_conn_ctx*>(p->data);
        assert(ctx);
        ctx->parse_header_complete_ = true;
        return 0;
    }

    static int on_message_complete(http_parser* _)
    {
        auto ctx = static_cast<http_conn_ctx*>(_->data);
        assert(ctx);
        switch (ctx->type_) {
        case HTTP_RESPONSE: {
            ctx->resp_.status_code   = _->status_code;
            ctx->resp_.version.major = _->http_major;
            ctx->resp_.version.minor = _->http_minor;
            break;
        }
        case HTTP_REQUEST: {
            ctx->request_->version.major = _->http_major;
            ctx->request_->version.minor = _->http_minor;
            break;
        }
        default:
            break;
        }

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
                ctx->some_body_.append(at, length);
            }
            else {
                if (!ctx->some_body_.empty()) {
                    ctx->resp_.body.write(ctx->some_body_.c_str(),
                                          ctx->some_body_.size());
                    ctx->some_body_.clear();
                }
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
        http_method method  = request_->method;
        std::string version = fmt::Sprintf(
            "HTTP/%d.%d", request_->version.major, request_->version.minor);
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

        size_t nparsed =
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

        request_->headers["Cache-Control"]   = kCacheControl;
        request_->headers["Accept"]          = kAccept;
        request_->headers["Accept-Language"] = kAcceptLanguage;
        request_->headers["User-Agent"]      = kUserAgent;

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

}  // namespace net
}  // namespace pp
