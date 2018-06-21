#pragma once
#include <boost/asio/io_service.hpp>
#include <boost/uuid/uuid.hpp>
#include <unordered_map>
#include <random>
#include <aware/aware.hpp>
#include <boost/signals2.hpp>

#include "aseba_node.h"

namespace mobsya {

class aseba_node_registery : public boost::asio::detail::service_base<aseba_node_registery> {
public:
    using node_id = uint16_t;
    using node_map = std::unordered_map<node_id, std::weak_ptr<aseba_node>>;

    aseba_node_registery(boost::asio::io_context& io_context);

    void add_node(std::shared_ptr<aseba_node> node);
    void remove_node(std::shared_ptr<aseba_node> node);
    void set_node_status(std::shared_ptr<aseba_node> node, aseba_node::status);
    void set_tcp_endpoint(const boost::asio::ip::tcp::endpoint& endpoint);
    void broadcast(const Aseba::Message& msg);

    node_map nodes() const;
    std::shared_ptr<aseba_node> node_from_id(node_id) const;


private:
    void update_discovery();
    void on_update_discovery_complete(const boost::system::error_code&);
    aware::contact::property_map_type build_discovery_properties() const;


    node_map::const_iterator find(std::shared_ptr<aseba_node> node) const;
    node_map::const_iterator find_from_native_id(aseba_node::node_id_t id) const;
    boost::uuids::uuid m_uid;
    node_map m_aseba_nodes;
    aware::announce_socket m_discovery_socket;
    aware::contact m_nodes_service_desc;
    mutable std::mutex m_discovery_mutex;
    mutable std::mutex m_nodes_mutex;

    boost::signals2::signal<void(std::shared_ptr<aseba_node>, node_id, aseba_node::status)>
        m_node_status_changed_signal;
    friend class node_status_monitor;


    struct id_generator {
        id_generator();
        node_id operator()();

    private:
        std::mt19937 gen;
        std::uniform_int_distribution<node_id> dis;
    } m_id_generator;
};


class node_status_monitor {
public:
    virtual ~node_status_monitor() {
        m_connection.disconnect();
    }
    virtual void node_changed(std::shared_ptr<aseba_node>, aseba_node_registery::node_id, aseba_node::status) = 0;

protected:
    void start_node_monitoring(aseba_node_registery& registery) {
        m_connection = registery.m_node_status_changed_signal.connect(
            boost::bind(&node_status_monitor::node_changed, this, boost::placeholders::_1, boost::placeholders::_2,
                        boost::placeholders::_3));
    }

private:
    boost::signals2::connection m_connection;
};


}  // namespace mobsya
