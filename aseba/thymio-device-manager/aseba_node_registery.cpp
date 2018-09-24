#include "aseba_node_registery.h"
#include "log.h"
#include <aware/aware.hpp>
#include <functional>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/asio/ip/host_name.hpp>

namespace mobsya {

aseba_node_registery::aseba_node_registery(boost::asio::io_context& io_context)
    : boost::asio::detail::service_base<aseba_node_registery>(io_context)
    , m_uid(boost::uuids::random_generator()())
    , m_discovery_socket(io_context)
    , m_nodes_service_desc("mobsya") {
    m_nodes_service_desc.name(fmt::format("Thymio Device Manager on {}", boost::asio::ip::host_name()));

    //  update_discovery();
}

void aseba_node_registery::add_node(std::shared_ptr<aseba_node> node) {
    std::unique_lock<std::mutex> lock(m_nodes_mutex);

    auto it = find(node);
    if(it == std::end(m_aseba_nodes)) {
        node_id id = node->uuid();
        if(id.is_nil())
            id = m_id_generator();
        it = m_aseba_nodes.insert({id, node}).first;
        lock.unlock();
        m_node_status_changed_signal(node, id, aseba_node::status::connected);

        mLogInfo("Adding node id: {} - Real id: {}", id, node->native_id());
    }
}

void aseba_node_registery::set_node_uuid(const std::shared_ptr<aseba_node>& node, const node_id& id) {
    std::unique_lock<std::mutex> lock(m_nodes_mutex);
    auto it = find(node);
    if(it != std::end(m_aseba_nodes)) {
        m_node_status_changed_signal(node, it->first, aseba_node::status::disconnected);
        m_aseba_nodes.erase(it);
    }
    m_aseba_nodes.insert({id, node});
    lock.unlock();
}

void aseba_node_registery::remove_node(const std::shared_ptr<aseba_node>& node) {
    {
        std::unique_lock<std::mutex> lock(m_nodes_mutex);

        mLogTrace("Removing node {}", node->friendly_name());
        auto it = find(node);
        if(it != std::end(m_aseba_nodes)) {
            node_id id = it->first;
            m_aseba_nodes.erase(it);
            lock.unlock();
            m_node_status_changed_signal(node, id, aseba_node::status::disconnected);
            lock.lock();
        }
    }
}

void aseba_node_registery::set_node_status(const std::shared_ptr<aseba_node>& node, aseba_node::status status) {
    {
        std::unique_lock<std::mutex> lock(m_nodes_mutex);
        auto it = find(node);
        if(it != std::end(m_aseba_nodes)) {
            node_id id = it->first;
            lock.unlock();
            mLogInfo("Changing node {} status to {} ", id, aseba_node::status_to_string(status));
            m_node_status_changed_signal(node, id, status);
            lock.lock();
        }
    }
}

aseba_node_registery::node_map aseba_node_registery::nodes() const {
    std::unique_lock<std::mutex> _(m_nodes_mutex);
    return m_aseba_nodes;
}

void aseba_node_registery::set_tcp_endpoint(const boost::asio::ip::tcp::endpoint& endpoint) {
    m_nodes_service_desc.endpoint(endpoint);
    update_discovery();
}

void aseba_node_registery::set_ws_endpoint(const boost::asio::ip::tcp::endpoint& endpoint) {
    m_ws_endpoint = endpoint;
    update_discovery();
}

void aseba_node_registery::update_discovery() {
    std::unique_lock<std::mutex> _(m_discovery_mutex);
    m_nodes_service_desc.properties(build_discovery_properties());

    m_discovery_socket.async_announce(
        m_nodes_service_desc,
        std::bind(&aseba_node_registery::on_update_discovery_complete, this, std::placeholders::_1));
}

void aseba_node_registery::on_update_discovery_complete(const boost::system::error_code& ec) {
    if(ec) {
        mLogError("Discovery : {}", ec.message());
    } else {
        mLogTrace("Discovery : update complete");
    }
}


aware::contact::property_map_type aseba_node_registery::build_discovery_properties() const {
    std::unique_lock<std::mutex> _(m_nodes_mutex);

    aware::contact::property_map_type map;
    map["uuid"] = boost::uuids::to_string(m_uid);
    if(m_ws_endpoint.port())
        map["ws-port"] = std::to_string(m_ws_endpoint.port());
    return map;
}

auto aseba_node_registery::find(const std::shared_ptr<aseba_node>& node) const -> node_map::const_iterator {
    for(auto it = std::begin(m_aseba_nodes); it != std::end(m_aseba_nodes); ++it) {
        if(it->second.expired())
            continue;
        if(it->second.lock() == node)
            return it;
    }
    return std::end(m_aseba_nodes);
}

auto aseba_node_registery::find_from_native_id(aseba_node::node_id_t id) const -> node_map::const_iterator {
    for(auto it = std::begin(m_aseba_nodes); it != std::end(m_aseba_nodes); ++it) {
        if(it->second.expired())
            continue;
        if(it->second.lock()->native_id() == id)
            return it;
    }
    return std::end(m_aseba_nodes);
}

std::shared_ptr<aseba_node> aseba_node_registery::node_from_id(const aseba_node_registery::node_id& id) const {
    std::unique_lock<std::mutex> _(m_nodes_mutex);
    auto it = m_aseba_nodes.find(id);
    if(it == std::end(m_aseba_nodes))
        return {};
    return it->second.lock();
}

node_status_monitor::~node_status_monitor() {
    m_connection.disconnect();
}

}  // namespace mobsya
