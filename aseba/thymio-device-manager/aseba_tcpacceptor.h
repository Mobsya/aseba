#pragma once
#include <aware/aware.hpp>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <queue>
#include <thread>

namespace mobsya {
class aseba_endpoint;
class aseba_tcp_acceptor {
public:
    aseba_tcp_acceptor(boost::asio::io_context& io_context);
    ~aseba_tcp_acceptor();
    void accept();

private:
    void monitor();
    void monitor_next();
    void do_accept();
    void push_contact(aware::contact contact);
    std::mutex m_queue_mutex;
    std::queue<aware::contact> m_pending_contacts;

    boost::asio::io_context& m_iocontext;
    std::thread m_monitor_thread;
    std::atomic_bool m_stopped;
    aware::contact m_contact;
    boost::asio::io_context m_monitor_ctx;
    aware::monitor_socket m_monitor;
    using known_ep = std::pair<std::string, uint16_t>;
    std::map<known_ep, std::weak_ptr<aseba_endpoint>> m_connected_endpoints;
    std::mutex m_endpoints_mutex;
};


}  // namespace mobsya
