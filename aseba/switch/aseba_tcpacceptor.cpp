#include "aseba_tcpacceptor.h"
#include <iostream>
#include "aseba_endpoint.h"
#include "log.h"

namespace mobsya {

static const std::map<std::string, aseba_endpoint::endpoint_type> endpoint_type_mapping = {
    {"Thymio II", aseba_endpoint::endpoint_type::simulated_tymio},
    {"Dummy Node", aseba_endpoint::endpoint_type::simulated_dummy_node}};

aseba_tcp_acceptor::aseba_tcp_acceptor(boost::asio::io_context& io_context)
    : m_iocontext(io_context), m_contact("aseba"), m_monitor(io_context), m_resolver(io_context) {}

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

        known_ep key;
        for(auto ar : m_resolver.resolve(m_contact.endpoint())) {
            key = known_ep{ar.host_name(), m_contact.endpoint().port()};
            auto it = m_connected_endpoints.find(key);
            if(it != std::end(m_connected_endpoints) && !it->second.expired()) {
                // mLogDebug("[tcp] {} already connected", m_contact.endpoint());
                do_accept();
                return;
            }
        }

        const auto& properties = m_contact.properties();
        int protocol_version = 5;
        aseba_endpoint::endpoint_type type = aseba_endpoint::endpoint_type::unknown;
        std::string type_str;

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
        session->tcp().connect(m_contact.endpoint(), ec);
        if(!ec) {
            mLogInfo("[tcp] New aseba node connected: {} on {} (protocol version : {}, type : {})", m_contact.name(),
                     m_contact.endpoint(), protocol_version, type_str);
            session->set_endpoint_name(m_contact.name());
            session->set_endpoint_type(type);
            session->start();
            m_connected_endpoints[key] = session;
        } else {
            mLogWarn("[tcp] Fail to connect to aseba node {} : {}", m_contact.name(), ec.message());
        }

        accept();
    });
}
}  // namespace mobsya
