#include "aseba_node_registery.h"
#include "aseba_endpoint.h"
#include "log.h"
#include "uuid_provider.h"
#include <aware/aware.hpp>
#include <functional>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/asio/ip/host_name.hpp>

namespace mobsya {

aseba_node_registery::aseba_node_registery(boost::asio::execution_context& io_context)
    : boost::asio::detail::service_base<aseba_node_registery>(static_cast<boost::asio::io_context&>(io_context))
    , m_service_uid(boost::asio::use_service<uuid_generator>(io_context).generate())
    , m_discovery_socket(static_cast<boost::asio::io_context&>(io_context))
    , m_nodes_service_desc("mobsya") {
    m_nodes_service_desc.name(fmt::format("Thymio Device Manager on {}", boost::asio::ip::host_name()));

    //  update_discovery();
}

void aseba_node_registery::add_node(std::shared_ptr<aseba_node> node) {
    remove_duplicated_node(node);
    auto it = find(node);
    if(it == std::end(m_aseba_nodes)) {
        node_id id = node->uuid();
        if(id.is_nil())
            id = boost::asio::use_service<uuid_generator>(get_io_context()).generate();
        it = m_aseba_nodes.insert({id, node}).first;

        restore_group_affiliation(*node);
        save_group_affiliation(*node);

        m_node_status_changed_signal(node, id, aseba_node::status::connected);
        mLogInfo("Adding node id: {} - Real id: {}", id, node->native_id());
    }
}

void aseba_node_registery::handle_node_uuid_change(const std::shared_ptr<aseba_node>& node) {
    auto it = find(node);
    if(it != std::end(m_aseba_nodes)) {
        m_node_status_changed_signal(node, it->first, aseba_node::status::disconnected);
        m_aseba_nodes.erase(it);
    }
    remove_duplicated_node(node);
    m_aseba_nodes.insert({node->uuid(), node});
    restore_group_affiliation(*node);
    save_group_affiliation(*node);
}

void aseba_node_registery::remove_node(const std::shared_ptr<aseba_node>& node) {
    mLogTrace("Removing node {}", node->friendly_name());
    auto it = find(node);
    if(it == std::end(m_aseba_nodes)) {
        return;
    }
    node_id id = it->first;
    m_aseba_nodes.erase(it);
    m_node_status_changed_signal(node, id, aseba_node::status::disconnected);
}

void aseba_node_registery::remove_duplicated_node(const std::shared_ptr<aseba_node>& node) {
    auto it = m_aseba_nodes.find(node->uuid());
    if(it == m_aseba_nodes.end())
        return;
    auto old = it->second.lock();
    if(old == node)
        return;
    mLogWarn("Removing duplicated node for {}", node->uuid());
    m_aseba_nodes.erase(it);
    if(old) {
        if(auto ep = old->endpoint()) {
            ep->remove_node(node->uuid());
        }
    }
}
void aseba_node_registery::save_group_affiliation(const aseba_node& node) {
    auto group = node.group();
    if(group) {
        m_ghost_groups.insert_or_assign(node.uuid(), last_known_node_group{group});
    }
}

void aseba_node_registery::restore_group_affiliation(const aseba_node& node) {
    auto it = m_ghost_groups.find(node.uuid());
    if(it == std::end(m_ghost_groups))
        return;
    auto ep = node.endpoint();
    if(ep && (ep->group() != it->second.group) && (!ep->group() || !ep->group()->has_state())) {
        it->second.group->attach_to_endpoint(node.endpoint());
    }
    m_ghost_groups.erase(it);
}

void aseba_node_registery::set_node_status(const std::shared_ptr<aseba_node>& node, aseba_node::status status) {
    auto it = find(node);
    if(it != std::end(m_aseba_nodes)) {
        node_id id = it->first;
        mLogInfo("Changing node {} status to {} ", id, aseba_node::status_to_string(status));
        m_node_status_changed_signal(node, id, status);
    }
}

aseba_node_registery::node_map aseba_node_registery::nodes() const {
    return m_aseba_nodes;
}

void aseba_node_registery::set_tcp_endpoint(const boost::asio::ip::tcp::endpoint& endpoint) {
    m_nodes_service_desc.endpoint(endpoint);
}

void aseba_node_registery::set_ws_endpoint(const boost::asio::ip::tcp::endpoint& endpoint) {
    m_ws_endpoint = endpoint;
}

void aseba_node_registery::set_discovery() {
    update_discovery();
}

void aseba_node_registery::set_tcp_endpoint_ul(const boost::asio::ip::tcp::endpoint& endpoint) {
    m_nodes_service_desc.endpoint(endpoint);
}

void aseba_node_registery::set_ws_endpoint_ul(const boost::asio::ip::tcp::endpoint& endpoint) {
    m_ws_endpoint = endpoint;
}

void aseba_node_registery::update_discovery() {

    if(m_updating_discovery) {
        m_discovery_needs_update = true;
        return;
    }
    m_nodes_service_desc.properties(build_discovery_properties());

    m_discovery_needs_update = false;
    m_updating_discovery = true;
    m_discovery_socket.async_announce(
        m_nodes_service_desc,
        std::bind(&aseba_node_registery::on_update_discovery_complete, this, std::placeholders::_1));
}

void aseba_node_registery::on_update_discovery_complete(const boost::system::error_code& ec) {
    if(ec) {
        mLogError("Discovery : {}", ec.message());
		m_discovery_needs_update = true;
    } else {
        mLogTrace("Discovery : update complete");
		m_updating_discovery = false;
    }
	
    if(m_discovery_needs_update) {
        boost::asio::post(boost::asio::get_associated_executor(this),
                          boost::bind(&aseba_node_registery::update_discovery, this));
    }
}


aware::contact::property_map_type aseba_node_registery::build_discovery_properties() const {
    aware::contact::property_map_type map;
    map["uuid"] = boost::uuids::to_string(m_service_uid);
    map["_ws-port"] = "8597"; //HACK adding some unused description to correct bug in avahi
    if(m_ws_endpoint.port())
        map["ws-port"] = std::to_string(m_ws_endpoint.port());
    mLogTrace("=> WS port discovery on {}", map["ws-port"]);
    map["_ws-port2"] = "8597"; //HACK adding some unused description to correct bug in avahi
    return map;
}

auto aseba_node_registery::find(const std::shared_ptr<aseba_node>& node) const -> node_map::const_iterator {
    for(auto it = std::begin(m_aseba_nodes); it != std::end(m_aseba_nodes); ++it) {
        if(it->second.lock() == node)
            return it;
    }
    return std::end(m_aseba_nodes);
}

auto aseba_node_registery::find_from_native_id(aseba_node::node_id_t id) const -> node_map::const_iterator {
    for(auto it = std::begin(m_aseba_nodes); it != std::end(m_aseba_nodes); ++it) {
        const auto n = it->second.lock();
        if(n && n->native_id() == id)
            return it;
    }
    return std::end(m_aseba_nodes);
}

std::shared_ptr<aseba_node> aseba_node_registery::node_from_id(const aseba_node_registery::node_id& id) const {
    const auto it = m_aseba_nodes.find(id);
    if(it == std::end(m_aseba_nodes))
        return {};
    return it->second.lock();
}

std::shared_ptr<mobsya::group> aseba_node_registery::group_from_id(const node_id& id) const {
    for(auto it = std::begin(m_aseba_nodes); it != std::end(m_aseba_nodes); ++it) {
        const auto n = it->second.lock();
        if(n && (n->uuid() == id || (n->group() && n->group()->uuid() == id)))
            return n->group();
    }
    return {};
}

void aseba_node_registery::register_endpoint(std::shared_ptr<aseba_endpoint> p) {
    auto it = std::find_if(m_endpoints.begin(), m_endpoints.end(),
                           [p](std::weak_ptr<aseba_endpoint> o) { return o.lock() == p; });
    if(it == m_endpoints.end()) {
        m_endpoints.push_back(p);
    }
}

void aseba_node_registery::unregister_expired_endpoints() {
    m_endpoints.erase(std::remove_if(m_endpoints.begin(), m_endpoints.end(), [](auto&& ep) { return ep.expired(); }),
                      m_endpoints.end());
}

void aseba_node_registery::disconnect_all_wireless_endpoints() {
    for(auto w : m_endpoints) {
        const auto ep = w.lock();
        if(ep && ep->is_wireless()) {
            ep->destroy();
        }
    }
}

node_status_monitor::~node_status_monitor() {
    m_connection.disconnect();
}


}  // namespace mobsya
