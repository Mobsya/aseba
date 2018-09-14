#include "aseba_tcpacceptor.h"
#include <iostream>
#include "aseba_endpoint.h"
#include "log.h"

namespace mobsya {

static const std::map<std::string, aseba_endpoint::endpoint_type> endpoint_type_mapping = {
    {"Thymio II", aseba_endpoint::endpoint_type::simulated_tymio},
    {"Dummy Node", aseba_endpoint::endpoint_type::simulated_dummy_node}};

aseba_tcp_acceptor::aseba_tcp_acceptor(boost::asio::io_context& io_context)
    : m_iocontext(io_context), m_contact("aseba"), m_monitor(io_context), m_resolver(io_context) {

    using tcp = boost::asio::ip::tcp;
    for(const auto suffix : {"", ".local"}) {
        tcp::resolver::query query(boost::asio::ip::host_name() + suffix, "", tcp::resolver::query::canonical_name);
        for(auto it = m_resolver.resolve(query); it != tcp::resolver::iterator(); ++it) {
            mLogInfo("Local Ip : {}", it->endpoint().address().to_string());
            m_local_ips.insert(it->endpoint().address());
        }
    }
}

void aseba_tcp_acceptor::accept() {
    mLogInfo("Waiting for aseba node on tcp");
    do_accept();
}

void aseba_tcp_acceptor::do_accept() {
    auto session = aseba_endpoint::create_for_tcp(m_iocontext);
    m_monitor.async_listen(m_contact, [this, session](boost::system::error_code ec) {
        if(ec) {
            // error ?
            return;
        }
        if(m_contact.empty() || m_contact.type() != "aseba") {
            do_accept();
            return;
        }

        if(m_local_ips.find(m_contact.endpoint().address()) == m_local_ips.end()) {
            mLogTrace("Ignoring remote endoint {} (expected: {})", m_contact.endpoint().address().to_string(),
                      boost::asio::ip::host_name());
            do_accept();
            return;
        }

        const auto key = known_ep{boost::asio::ip::host_name(), m_contact.endpoint().port()};
        {
            std::unique_lock<std::mutex> _(m_endpoints_mutex);
            auto it = m_connected_endpoints.find(key);
            if(it != std::end(m_connected_endpoints) && !it->second.expired()) {
                mLogTrace("[tcp] {} already connected", m_contact.endpoint());
                do_accept();
                return;
            }
        }

        session->tcp().async_connect(
            m_contact.endpoint(), [this, contact = m_contact, session, key = key](boost::system::error_code ec) {
                const auto& properties = contact.properties();

                int protocol_version = 5;
                aseba_endpoint::endpoint_type type = aseba_endpoint::endpoint_type::unknown;
                std::string type_str;
                if(ec) {
                    mLogWarn("[tcp] Fail to connect to aseba node {} : {}", contact.name(), ec.message());
                    return;
                }
                {
                    std::unique_lock<std::mutex> _(m_endpoints_mutex);
                    auto it = m_connected_endpoints.find(key);
                    if(it != std::end(m_connected_endpoints) && !it->second.expired()) {
                        mLogTrace("[tcp] {} already connected", m_contact.endpoint());
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

                session->set_endpoint_name(contact.name());
                session->set_endpoint_type(type);
                session->start();
            });
        accept();
    });
}
}  // namespace mobsya
