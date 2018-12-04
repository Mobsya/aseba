#include "group.h"
#include <aseba/common/utils/utils.h>
#include "error.h"
#include "property.h"
#include "aseba_endpoint.h"
#include "uuid_provider.h"
#include "aseba_property.h"

namespace mobsya {

group::group(boost::asio::io_context& context)
    : m_context(context), m_uuid(boost::asio::use_service<uuid_generator>(m_context).generate()) {}


std::shared_ptr<group> group::make_group_for_endpoint(boost::asio::io_context& context,
                                                      std::shared_ptr<aseba_endpoint> initial_ep) {
    auto g = std::make_shared<group>(context);
    g->attach_to_endpoint(initial_ep);
    return g;
}

bool group::has_state() const {
    return m_endpoints.size() > 1 || !m_events_table.empty() || !m_shared_variables.empty();
}

bool group::has_node(const node_id& node) const {
    for(auto&& n : nodes()) {
        if(n->uuid() == node)
            return true;
    }
    return false;
}

node_id group::uuid() const {
    return m_uuid;
}

std::vector<std::shared_ptr<aseba_endpoint>> group::endpoints() const {
    std::vector<std::shared_ptr<aseba_endpoint>> set;
    for(auto&& ptr : m_endpoints) {
        auto ep = ptr.lock();
        if(!ep)
            continue;
        set.push_back(ep);
    }
    return set;
}

void group::attach_to_endpoint(std::shared_ptr<aseba_endpoint> ep) {
    auto eps = endpoints();
    auto it = std::find(eps.begin(), eps.end(), ep);
    if(it == eps.end()) {
        m_endpoints.push_back(ep);
    }

    ep->set_group(this->shared_from_this());
    ep->set_events_table(m_events_table);

    for(auto&& node : ep->nodes()) {
        node->set_status(node->get_status());
    }
}

std::vector<std::shared_ptr<aseba_node>> group::nodes() const {
    std::vector<std::shared_ptr<aseba_node>> nodes;
    for(auto&& e : endpoints()) {
        auto endpoint_nodes = e->nodes();
        nodes.reserve(endpoint_nodes.size());
        std::copy(endpoint_nodes.begin(), endpoint_nodes.end(), std::back_inserter(nodes));
    }
    return nodes;
}

void group::emit_events(const properties_map& map, write_callback&& cb) {

    // call the callback when all endpoints send the events
    // using the deleter of a shared_ptr owning the callback
    const auto deleter = [](write_callback* cb) {
        if(*cb)
            (*cb)({});
        delete cb;
    };
    const auto ptr = std::shared_ptr<write_callback>(new write_callback(std::move(cb)), deleter);

    for(const auto& ep : endpoints()) {
        ep->emit_events(map, [cb = ptr](boost::system::error_code ec) mutable {
            // If one endpoint fails, report the error of that endpoint ( and ignore further errors)
            if(ec && cb && *cb) {
                (*cb)(ec);
                cb.reset();
            }
        });
    }
}

void group::on_event_received(std::shared_ptr<aseba_endpoint> source_ep, const node_id& source_node,
                              const group::properties_map& events) {
    // Broadcast the events to other endpoints
    for(const auto& ep : endpoints()) {
        if(ep == source_ep)
            continue;
        ep->emit_events(events, {});
    }
}

boost::system::error_code group::set_shared_variables(const properties_map& map) {
    m_shared_variables = map;
    for(auto&& ep : endpoints()) {
        ep->set_shared_variables(map);
    }
    m_variables_changed_signal(shared_from_this(), m_shared_variables);
    return {};
}


boost::system::error_code group::set_events_table(const events_table& events) {
    m_events_table = events;
    for(auto&& ep : endpoints()) {
        ep->set_events_table(events);
    }
    send_events_table();
    return {};
}

group::events_table group::get_events_table() const {
    return m_events_table;
}

void group::send_events_table() {
    m_events_changed_signal(shared_from_this(), m_events_table);
}


}  // namespace mobsya