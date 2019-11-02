#include <io/io_event_loop.h>
#include <net/http/net_http_client.h>

#include "http_conn_ctx.h"

using namespace std::tr1::placeholders;

namespace pp {
namespace net {
#define __user_data(ptr, type) ptr->user_data().any_cast<type>()

http_client::http_client(io::event_loop *loop) : loop_(loop) {}

http_request_ref
http_client::new_request(const std::string &url, http_method method,
                         const http_response_handler &resp_handler) {
  auto request = std::make_shared<http_request>();
  request->method = method;
  request->url = url;
  request->version = {1, 1};
  request->resp_handler_ = resp_handler;
  request->handler_called_ = false;
  request->abort_ = false;

  return request;
}

void http_client::write_request_line(const tcp_conn_wref &conn,
                                     const http_conn_ctx_wref &ctx) {
  auto rconn = conn.lock();
  auto rctx = ctx.lock();

  assert(rconn && rctx);

  // next send header
  rconn->data_write_finished(
      std::bind(&http_client::write_headers, this, conn, ctx));

  auto request_line = rctx->build_request_line();
  rconn->write(request_line.c_str(), request_line.size());
  rconn->write("\r\n", 2);
}

void http_client::write_headers(const tcp_conn_wref &conn,
                                const http_conn_ctx_wref &ctx) {
  auto rconn = conn.lock();
  auto rctx = ctx.lock();

  assert(rconn && rctx);

  rconn->data_write_finished(
      std::bind(&http_client::write_body, this, conn, ctx));
  auto header_line = rctx->build_request_header();
  rconn->write(header_line.c_str(), header_line.size());
  rconn->async_read();
}

void http_client::write_body(const tcp_conn_wref &conn,
                             const http_conn_ctx_wref &ctx) {
  auto rconn = conn.lock();
  auto rctx = ctx.lock();

  assert(rconn && rctx);

  char some[4096] = {0};
  int len = (int)rctx->build_some_body(some, sizeof some);
  if (len == 0) {
    rconn->data_write_finished(nullptr);
  }
  rconn->write(some, len);
}

void http_client::remove_client(const http_conn_ctx_ref &ctx) {
  assert(ctx);
  auto found = client_list_.find(ctx);
  assert(found != client_list_.end());
  client_list_.erase(found);
}

void http_client::add_client(const tcp_client_ref &client,
                             const http_conn_ctx_ref &ctx) {
  client_list_[ctx] = client;
}

void http_client::handle_resp(const http_response *resp, bool finished,
                              const errors::error_code &error) {
  auto request = resp->request.lock();

  if (request && request->resp_handler_) {
    request->resp_handler_((http_response *)resp, finished, error);
  }
}

void http_client::run(const http_request_ref &request,
                      errors::error_code &error) {
  assert(request);

  auto ctx = std::make_shared<http_conn_ctx>(request, HTTP_RESPONSE);

  int ret = ctx->parse_url();
  if (ret != 0) {
    // bad url
    error = hht_make_error_code(
        static_cast<errors::error>(errors::error::NET_ERROR));
    error.suffix_msg("bad url");
    return;
  }

  int off = ctx->parse_url_.field_data[UF_HOST].off;
  int len = ctx->parse_url_.field_data[UF_HOST].len;
  int port = ctx->parse_url_.port == 0 ? 80 : ctx->parse_url_.port;
  std::string host = std::string(ctx->request_->url.c_str() + off, len);

  auto client = std::make_shared<tcp_client>(loop_, addr(host, port));
  ctx->request_->headers["Host"] = host;

  std::weak_ptr<http_conn_ctx> wkctx = ctx;
  client->dial_done(
      std::bind(&http_client::send_request, this, _1, _2, _3, wkctx));
  client->message_recved(std::bind(&http_client::recv_resp, this, _1, _2, _3));

  client->set_user_data(http_conn_ctx_wref(ctx));

  client->dial(0);
  add_client(client, ctx);
}

void http_client::cancel(const http_request_ref &request) {
  std::weak_ptr<http_request> wreq(request);
  loop_->run_in_loop([&, wreq]() {
    http_request_ref req = wreq.lock();
    if (!req) {
      return;
    }

    for (auto client = client_list_.begin(); client != client_list_.end();
         client++) {
      auto ctx = client->first;
      if (ctx->request_ == req) {
        auto cli = client->second;
        ctx->request_->abort_ = true;
        ctx->error_ = hht_make_error_code(
            static_cast<std::errc>(std::errc::connection_aborted));
        cli->close();
        return;
      }
    }
  });
}

void http_client::send_request(const net::tcp_conn_ref &conn,
                               const _time::time &now,
                               const errors::error_code &error,
                               const std::weak_ptr<http_conn_ctx> &wkctx) {
  // auto ctx = __user_data(conn, http_conn_ctx_wref).lock();
  auto ctx = wkctx.lock();
  assert(ctx);

  auto request = ctx->request_;
  errors::error_code err;

  // connecting failed
  if (error) {
    handle_resp(&ctx->resp_, false, error);
    remove_client(ctx);
    return;
  }
  // conn closed
  if (!conn->connected()) {
    bool finished = true;
    err = ctx->error_;
    if (err) {
      finished = false;
    }
    handle_resp(&ctx->resp_, finished, err);
    remove_client(ctx);
    return;
  }
  write_request_line(conn, ctx);
}

void http_client::recv_resp(const tcp_conn_ref &conn, bytes::Buffer &buffer,
                            const _time::time &time) {
  auto ctx = __user_data(conn, http_conn_ctx_wref).lock();
  assert(ctx);
  auto req = ctx->request_;

  // before parsing,we must post a read,
  // if not do this, when conn closed, we can't recv close notify
  // when parsing, error occured
  conn->async_read();
  if (ctx->error_.value() != 0) {
    conn->close();
    return;
  }

  ctx->parsing_http_msg(buffer);
  if (ctx->parse_header_complete_ && !req->handler_called_) {
    handle_resp(&ctx->resp_, false, errors::error_code());
    req->handler_called_ = true;
  }
  if (ctx->resp_.body.is_nil() && req->handler_called_) {
    ctx->parse_complete_ = true;
    conn->close();
    return;
  }
  if (ctx->parse_complete_) {
    if (!ctx->some_body_.empty() && !ctx->resp_.body.is_nil()) {
      ctx->resp_.body.write(ctx->some_body_.data(), ctx->some_body_.size(),
                            ctx->error_);
    }
    conn->close();
  }
}
} // namespace net
} // namespace pp
