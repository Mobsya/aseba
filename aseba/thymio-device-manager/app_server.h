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
        : m_io_context(io_context), m_acceptor(io_context, tcp::endpoint(tcp::v6(), port)) {
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
            bool same_machine = mobsya::endpoint_is_local(endpoint->tcp_socket().remote_endpoint());
            bool same_network = mobsya::endpoint_is_network_local(endpoint->tcp_socket().remote_endpoint());
            mLogInfo("New {} connection from {} {}",
                     same_network ? "Local" : "Remote",
                     endpoint->tcp_socket().remote_endpoint().address().to_string(),
                     error.message());

            // Drop connections to non-locale machines unless the TDM
            // was started with --allow-remote-connections
            if(!same_machine && !m_allow_remote_connections) {
                mLogWarn("Connection from {} dropped: remote connections not allowed",
                         endpoint->tcp_socket().remote_endpoint().address().to_string());
            }
            else if(!error) {
                if(same_machine)
                    endpoint->set_is_machine_local();
                if(same_network)
                    endpoint->set_is_network_local();
                endpoint->start();
            }
            this->accept();
        });
    }

    void allow_remote_connections(bool allow) {
        m_allow_remote_connections = allow;
    }

private:
    boost::asio::io_context& m_io_context;
    tcp::acceptor m_acceptor;
    bool m_allow_remote_connections = false;
};

extern template class application_server<websocket_t>;
extern template class application_server<mobsya::tcp::socket>;

}  // namespace mobsya
