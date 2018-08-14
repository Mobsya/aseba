#pragma once
#include <aware/aware.hpp>
#include <map>
#include <memory>

namespace mobsya {
class aseba_endpoint;
class aseba_tcp_acceptor {
public:
    aseba_tcp_acceptor(boost::asio::io_context& io_context);
    void accept();

private:
    boost::asio::io_context& m_iocontext;
    aware::contact m_contact;
    aware::monitor_socket m_monitor;
    boost::asio::ip::tcp::resolver m_resolver;
    using known_ep = std::pair<std::string, uint16_t>;
    std::map<known_ep, std::weak_ptr<aseba_endpoint>> m_connected_endpoints;
};


}  // namespace mobsya
