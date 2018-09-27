#pragma once
#include <aware/aware.hpp>
#include <map>
#include <memory>
#include <mutex>
#include <set>

namespace mobsya {
class aseba_endpoint;
class aseba_tcp_acceptor {
public:
    aseba_tcp_acceptor(boost::asio::io_context& io_context);
    void accept();

private:
    void do_accept();
    boost::asio::io_context& m_iocontext;
    aware::contact m_contact;
    aware::monitor_socket m_monitor;
    using known_ep = std::pair<std::string, uint16_t>;
    std::map<known_ep, std::weak_ptr<aseba_endpoint>> m_connected_endpoints;
    std::mutex m_endpoints_mutex;
};


}  // namespace mobsya
