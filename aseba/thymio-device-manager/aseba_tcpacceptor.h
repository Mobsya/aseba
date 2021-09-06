#pragma once
#include <aware/aware.hpp>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <optional>
#include <queue>
#include <thread>
#include <boost/asio/deadline_timer.hpp>

namespace mobsya {
class aseba_endpoint;
class aseba_device;
class aseba_tcp_acceptor : public boost::asio::detail::service_base<aseba_tcp_acceptor> {
public:
    aseba_tcp_acceptor(boost::asio::io_context& io_context);
    aseba_tcp_acceptor(boost::asio::execution_context& io_context);
    ~aseba_tcp_acceptor();
    void accept();
    void free_endpoint(const aseba_device* ep);

private:
    void monitor();
    void monitor_next();
    void do_accept();
    void do_accept_contact(aware::contact contact);
    void do_accept_endpoint(boost::asio::ip::tcp::endpoint endpoint);


    void push_contact(aware::contact contact);
    std::mutex m_queue_mutex;
    std::vector<aware::contact> m_pending_contacts;
    std::set<aware::contact> m_disconnected_contacts;

    boost::asio::io_context& m_iocontext;
    std::thread m_monitor_thread;
    std::atomic_bool m_stopped;
    aware::contact m_contact;
    boost::asio::io_context m_monitor_ctx;
    std::optional<aware::monitor_socket> m_monitor;
    using known_ep = std::pair<std::string, uint16_t>;
    std::map<known_ep, std::weak_ptr<aseba_endpoint>> m_connected_endpoints;
    std::map<const aseba_device*, aware::contact> m_known_contacts;

    std::mutex m_endpoints_mutex;
    boost::asio::deadline_timer m_active_timer;
};


}  // namespace mobsya
