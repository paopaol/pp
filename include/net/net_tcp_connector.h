#ifndef NET_TCP_CONNECTOR_H
#define NET_TCP_CONNECTOR_H

#include <errors/hht_error.h>
#include <hht.h>
#include <io/io_event_fd.h>
#include <net/net.h>
#include <net/net_socket_accpeter.h>

namespace pp {
namespace io {
    class io::event_loop;
}  // namespace io
}  // namespace pp

namespace pp {
namespace net {
    class tcp_connector {
    public:
        //! Default constructor
        tcp_connector(io::event_loop* loop, const addr& addr);

        //! Destructor
        virtual ~tcp_connector() noexcept;

        void set_new_conn_handler(const new_conn_handler& handler);
        int  start_connect(errors::error_code& error);
        void connect_done();

    private:
        //! Copy constructor
        tcp_connector(const tcp_connector& other);

        //! Move constructor
        tcp_connector(tcp_connector&& other) noexcept;

        //! Copy assignment operator
        tcp_connector& operator=(const tcp_connector& other);

        //! Move assignment operator
        tcp_connector& operator=(tcp_connector&& other) noexcept;

        io::event_loop*    loop_;
        net::socket        socket_;
        io::event_fd_ref   conn_fd_;
        new_conn_handler   new_conn_handler_;
        errors::error_code error_;
        int                type;
        addr               addr_;
    };
    typedef std::shared_ptr<tcp_connector> tcp_connector_ref;
}  // namespace net
}  // namespace pp

#endif /* NET_WIN_IOCP_CONNECTOR_H */
