#include <io/io_event_loop.h>
#include <net/net_tcp_client.h>

#include <functional>

using namespace std::tr1::placeholders;

namespace pp {
namespace net {
    tcp_client::tcp_client(io::event_loop* loop, const addr& addr)
        : loop_(loop),
          addr_(addr),
          tcp_connector_(std::make_shared<tcp_connector>(loop, addr))
    {
        tcp_connector_->set_new_conn_handler(
            std::bind(&tcp_client::conn_connected, this, _1, _2));
    }
    tcp_client::~tcp_client() {}

    void tcp_client::dial_done(const connection_handler& handler)
    {
        handle_connection_ = handler;
    }

    void tcp_client::message_recved(const message_handler& handler)
    {
        handle_recv_data_ = handler;
    }

    void tcp_client::data_write_finished(const write_finished_handler& handler)
    {
        handle_write_finished_ = handler;
    }

    void tcp_client::dial(_time::Duration timeout)
    {
        loop_->run_in_loop([&]() {
            errors::error_code error;
            tcp_connector_->start_connect(timeout, error);
        });
    }

    void tcp_client::write(char* data, int len)
    {
        tcp_conn_->write(data, len);
    }

    void tcp_client::close()
    {
        tcp_conn_->shutdown();
    }

    void tcp_client::set_user_data(const pp::Any& any)
    {
        any_ = any;
    }

    void tcp_client::conn_connected(int fd, const errors::error_code& error)
    {
        tcp_connector_->detach_loop();
        tcp_conn_ = std::make_shared<tcp_conn>(loop_, fd, error);

        tcp_conn_->connected(handle_connection_);
        tcp_conn_->closed(std::bind(&tcp_client::conn_closed, this, _1, _2));
        tcp_conn_->data_recved(handle_recv_data_);
        tcp_conn_->data_write_finished(handle_write_finished_);
        tcp_conn_->set_user_data(any_);

        // although,the conn is established here,but it maybe
        // a bad socket,eg, connect timeout,connect failed.
        // so error is not 0.
        tcp_conn_->connect_established(error);
    }

    void tcp_client::conn_closed(const net::tcp_conn_ref&  conn,
                                 const errors::error_code& error)
    {
        tcp_conn_->connect_destroyed(error);
    }

}  // namespace net
}  // namespace pp
