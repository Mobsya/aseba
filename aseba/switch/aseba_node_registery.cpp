#include "aseba_node_registery.h"
#include "log.h"
#include <aware/aware.hpp>
#include <functional>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace mobsya {

aseba_node_registery::aseba_node_registery(boost::asio::io_context& io_context)
    : boost::asio::detail::service_base<aseba_node_registery>(io_context)
    , m_uid(boost::uuids::random_generator()())
    , m_discovery_socket(io_context)
    , m_nodes_service_desc("_mobsya") {
    m_nodes_service_desc.name("Thymio Discovery service");

    update_discovery();
}

void aseba_node_registery::add_node(std::shared_ptr<aseba_node> node) {
    auto it = find(node);
    if(it == std::end(m_aseba_nodes)) {
        node_id id = 0;
        do {
            id = m_id_generator();
            it = m_aseba_nodes.find(id);
        } while(it != std::end(m_aseba_nodes));
        it = m_aseba_nodes.insert({id, node}).first;

        mLogInfo("Adding node id: {} - Real id: {}", id, node->native_id());
    }
    update_discovery();
}

void aseba_node_registery::remove_node(std::shared_ptr<aseba_node> node) {
    mLogInfo("Removing node");
    auto it = find(node);
    if(it != std::end(m_aseba_nodes)) {
        m_aseba_nodes.erase(it);
    }
    update_discovery();
}

void aseba_node_registery::set_node_status(std::shared_ptr<aseba_node> node, aseba_node::status) {
    mLogInfo("Changing node status");
    update_discovery();
}

void aseba_node_registery::set_tcp_endpoint(const boost::asio::ip::tcp::endpoint& endpoint) {
    m_nodes_service_desc.endpoint(endpoint);
    update_discovery();
}

void aseba_node_registery::update_discovery() {
    std::unique_lock _(m_discovery_mutex);
    m_nodes_service_desc.properties(build_discovery_properties());

    m_discovery_socket.async_announce(
        m_nodes_service_desc,
        std::bind(&aseba_node_registery::on_update_discovery_complete, this, std::placeholders::_1));
}

void aseba_node_registery::on_update_discovery_complete(const boost::system::error_code& ec) {
    mLogError("Discovery : {}", ec.message());
}


aware::contact::property_map_type aseba_node_registery::build_discovery_properties() const {
    aware::contact::property_map_type map;
    map["uuid"] = boost::uuids::to_string(m_uid);
    for(auto it = std::begin(m_aseba_nodes); it != std::end(m_aseba_nodes); ++it) {
        if(it->second.expired())
            continue;
        const auto ptr = it->second.lock();
        if(!ptr)
            continue;

        aseba_node::status s = ptr->get_status();
        std::string key = fmt::format("node-{}", it->first);
        std::string value = fmt::format("{}-{}", int(s), aseba_node::status_to_string(s));
        map.emplace(key, value);
    }
    return map;
}

auto aseba_node_registery::find(std::shared_ptr<aseba_node> node) const -> node_map::const_iterator {
    for(auto it = std::begin(m_aseba_nodes); it != std::end(m_aseba_nodes); ++it) {
        if(it->second.expired())
            continue;
        if(it->second.lock() == node)
            return it;
    }
    return std::end(m_aseba_nodes);
}


aseba_node_registery::id_generator::id_generator()
    : gen(std::random_device()()), dis(1, std::numeric_limits<node_id>::max()) {}

aseba_node_registery::node_id aseba_node_registery::id_generator::operator()() {
    return dis(gen);
}

}  // namespace mobsya
