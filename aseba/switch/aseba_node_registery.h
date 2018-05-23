#pragma once
#include <boost/asio/io_service.hpp>
#include <boost/uuid/uuid.hpp>
#include <unordered_map>
#include <random>
#include <aware/aware.hpp>
#include "aseba_node.h"

namespace mobsya {

class aseba_node_registery : public boost::asio::detail::service_base<aseba_node_registery> {
public:
    using node_id = uint32_t;
    aseba_node_registery(boost::asio::io_context& io_context);

    void add_node(std::shared_ptr<aseba_node> node);
    void remove_node(std::shared_ptr<aseba_node> node);
    void set_node_status(std::shared_ptr<aseba_node> node, aseba_node::status);
    void set_tcp_endpoint(const boost::asio::ip::tcp::endpoint& endpoint);

private:
    void update_discovery();
    void on_update_discovery_complete(const boost::system::error_code&);
    aware::contact::property_map_type build_discovery_properties() const;

    using node_map = std::unordered_map<node_id, std::weak_ptr<aseba_node>>;

    node_map::const_iterator find(std::shared_ptr<aseba_node> node) const;
    boost::uuids::uuid m_uid;
    node_map m_aseba_nodes;
    aware::announce_socket m_discovery_socket;
    aware::contact m_nodes_service_desc;
    mutable std::mutex m_discovery_mutex;


    struct id_generator {
        id_generator();
        node_id operator()();

    private:
        std::mt19937 gen;
        std::uniform_int_distribution<node_id> dis;
    } m_id_generator;
};


}  // namespace mobsya
