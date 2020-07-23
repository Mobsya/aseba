#pragma once
#include <boost/asio.hpp>
#include "app_endpoint.h"
#include "log.h"
#include "interfaces.h"

namespace mobsya {
using tcp = boost::asio::ip::tcp;

template <typename socket_type>
class application_server {
public:
    application_server(boost::asio::io_context& io_context, uint16_t port = 0)
        : m_io_context(io_context), m_acceptor(io_context, tcp::endpoint(boost::asio::ip::make_address("127.0.0.1"), port)) {
        // Make sure we accept both ipv4 + ipv6
        boost::asio::ip::v6_only opt(false);
        boost::system::error_code ec;
        m_acceptor.set_option(opt, ec);
        m_acceptor.listen(boost::asio::socket_base::max_listen_connections);
    }

    tcp::acceptor::endpoint_type endpoint() const {
        return m_acceptor.local_endpoint();
    }

    void accept() {
        auto endpoint = std::make_shared<application_endpoint<socket_type>>(m_io_context);
        m_acceptor.async_accept(endpoint->tcp_socket(), [this, endpoint](const boost::system::error_code& error) {
            mLogInfo("New connection from {} {}", endpoint->tcp_socket().remote_endpoint().address().to_string(),
                     error.message());
            if(!error) {
                endpoint->set_local(mobsya::endpoint_is_local(endpoint->tcp_socket().remote_endpoint()));
                endpoint->start();
            }
            this->accept();
        });
    }

private:
    boost::asio::io_context& m_io_context;
    tcp::acceptor m_acceptor;
};

extern template class application_server<websocket_t>;
extern template class application_server<mobsya::tcp::socket>;

}  // namespace mobsya
