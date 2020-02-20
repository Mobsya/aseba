#include "aseba_tcpacceptor.h"
#include <iostream>
#include <regex>
#include "aseba_endpoint.h"
#include "log.h"
#include "interfaces.h"

namespace mobsya {

static const std::map<std::string, aseba_endpoint::endpoint_type> endpoint_type_mapping = {
    {"Thymio II", aseba_endpoint::endpoint_type::simulated_thymio},
    {"Dummy Node", aseba_endpoint::endpoint_type::simulated_dummy_node}};

aseba_tcp_acceptor::aseba_tcp_acceptor(boost::asio::io_context& io_context)
    : boost::asio::detail::service_base<aseba_tcp_acceptor>(io_context)
    , m_iocontext(io_context)
    , m_stopped(false)
    , m_contact("aseba")
    , m_monitor(m_monitor_ctx)
    , m_active_timer(io_context) {

    m_monitor_thread = std::thread(boost::bind(&aseba_tcp_acceptor::monitor, this));
}

aseba_tcp_acceptor::aseba_tcp_acceptor(boost::asio::execution_context& io_context)
    : aseba_tcp_acceptor(static_cast<boost::asio::io_context&>(io_context)) {}

aseba_tcp_acceptor::~aseba_tcp_acceptor() {
    m_stopped.store(true);
    m_monitor_ctx.stop();
    m_monitor_thread.join();
}

void aseba_tcp_acceptor::free_endpoint(const aseba_device* ep) {
    auto it = m_known_contacts.find(ep);
    if(it != m_known_contacts.end()) {
        m_disconnected_contacts.insert(it->second);
        m_known_contacts.erase(it);
        boost::asio::post(m_iocontext, boost::bind(&aseba_tcp_acceptor::do_accept, this));
    }
}

void aseba_tcp_acceptor::monitor() {
    monitor_next();
    m_monitor_ctx.run();
}

void aseba_tcp_acceptor::monitor_next() {
    m_monitor.async_listen(m_contact, [this](boost::system::error_code ec) {
        if(ec) {
            mLogError("[tcp] Error {} - {}", ec.value(), ec.message());
            return;
        }
        mLogTrace("New contact {} : {}", m_contact.name(), m_contact.domain());
        this->push_contact(m_contact);
        boost::asio::post(m_iocontext, boost::bind(&aseba_tcp_acceptor::do_accept, this));
        if(!m_stopped.load())
            monitor_next();
    });
}

void aseba_tcp_acceptor::accept() {
    mLogInfo("Waiting for aseba node on tcp");
    do_accept();
}

std::string remove_host_from_name(const std::string& str) {
    return std::regex_replace(str, std::regex(" on [^ ]+$"), std::string{});
}

void aseba_tcp_acceptor::do_accept() {
    std::unique_lock<std::mutex> queue_mutex_lock(m_queue_mutex);
    if(m_pending_contacts.empty()) {
        return;
    }
    aware::contact contact = m_pending_contacts.front();
    m_pending_contacts.erase(m_pending_contacts.begin());
    queue_mutex_lock.unlock();
    do_accept_contact(contact);
}


void aseba_tcp_acceptor::do_accept_contact(aware::contact contact) {
    auto session = aseba_endpoint::create_for_tcp(m_iocontext);
    if(contact.empty() || contact.type() != "aseba") {
        boost::asio::post(m_iocontext, boost::bind(&aseba_tcp_acceptor::do_accept, this));
        return;
    }

    if(!mobsya::endpoint_is_local(contact.endpoint()) && contact.name().find("Not A Thymio 3") != 0) {
        mLogTrace("Ignoring remote endoint {} ({}) (expected: {})", contact.name(),
                  contact.endpoint().address().to_string(), boost::asio::ip::host_name());
        boost::asio::post(m_iocontext, boost::bind(&aseba_tcp_acceptor::do_accept, this));
        return;
    }

    const auto key = known_ep{boost::asio::ip::host_name(), contact.endpoint().port()};
    {
        std::unique_lock<std::mutex> _(m_endpoints_mutex);
        auto it = m_connected_endpoints.find(key);
        if(it != std::end(m_connected_endpoints) && !it->second.expired()) {
            mLogTrace("[tcp] {} already connected", contact.endpoint());
            boost::asio::post(m_iocontext, boost::bind(&aseba_tcp_acceptor::do_accept, this));
            return;
        }
    }

    m_known_contacts.emplace(session->device(), contact);
    session->tcp().async_connect(
        contact.endpoint(), [this, contact = std::move(contact), session, key = key](boost::system::error_code ec) {
            const auto& properties = contact.properties();
            int protocol_version = 5;
            aseba_endpoint::endpoint_type type = aseba_endpoint::endpoint_type::unknown;
            std::string type_str;
            if(ec) {
                mLogWarn("[tcp] Fail to connect to aseba node {} {} : {}", contact.name(),
                         contact.endpoint().address().to_string(), ec.message());
                boost::asio::post(m_iocontext, [this] { this->accept(); });
                return;
            }


            // remove the contact from the list of disconnected contact
            {
                auto it = std::find_if(m_disconnected_contacts.begin(), m_disconnected_contacts.end(),
                                       [contact](auto& other) { return contact == other; });
                if(it != m_disconnected_contacts.end())
                    m_disconnected_contacts.erase(it);
            }

            {
                std::unique_lock<std::mutex> _(m_endpoints_mutex);
                auto it = m_connected_endpoints.find(key);
                if(it != std::end(m_connected_endpoints) && !it->second.expired()) {
                    mLogTrace("[tcp] {} already connected", contact.endpoint());
                    boost::asio::post(m_iocontext, [this] { this->accept(); });
                    return;
                }
                m_connected_endpoints[key] = session;
            }
            mLogInfo("[tcp] New aseba node connected: {} on {} (protocol version : {}, type : {})", contact.name(),
                     contact.endpoint(), protocol_version, type_str);

            auto it = properties.find("type");
            if(it != std::end(properties)) {
                type_str = it->second;
                auto needle = endpoint_type_mapping.find(type_str);
                if(needle != std::end(endpoint_type_mapping))
                    type = needle->second;
            }
            it = properties.find("protovers");
            if(it != std::end(properties))
                protocol_version = std::atoi(it->second.c_str());

            session->set_endpoint_name(remove_host_from_name(contact.name()));
            session->set_endpoint_type(type);
            session->start();
            boost::asio::post(m_iocontext, [this] { this->accept(); });
        });
}  // namespace mobsya

void aseba_tcp_acceptor::push_contact(aware::contact contact) {
    std::unique_lock<std::mutex> _(m_queue_mutex);
    m_pending_contacts.push_back(std::move(contact));
}


}  // namespace mobsya
