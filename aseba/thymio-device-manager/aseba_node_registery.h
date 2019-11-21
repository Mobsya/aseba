#pragma once
#include <boost/asio/io_service.hpp>
#include <unordered_map>
#include <random>
#include <chrono>
#include <aware/aware.hpp>
#include <boost/signals2.hpp>
#include <range/v3/view/filter.hpp>
#include <range/v3/view/transform.hpp>
#include <range/v3/algorithm/find_if.hpp>
#include "aseba_node.h"
#include "node_id.h"
#include "group.h"
#include "aseba_endpoint.h"

namespace mobsya {

class aseba_node_registery : public boost::asio::detail::service_base<aseba_node_registery> {
public:
    using node_id = mobsya::node_id;
    using node_map = std::unordered_map<node_id, std::weak_ptr<aseba_node>>;
    using group_map = std::unordered_map<node_id, std::shared_ptr<group>>;

    aseba_node_registery(boost::asio::execution_context& ctx);

    void add_node(std::shared_ptr<aseba_node> node);
    void remove_node(const std::shared_ptr<aseba_node>& node);
    void set_node_status(const std::shared_ptr<aseba_node>& node, aseba_node::status);
    void handle_node_uuid_change(const std::shared_ptr<aseba_node>& node);

    void set_tcp_endpoint(const boost::asio::ip::tcp::endpoint& endpoint);
    void set_ws_endpoint(const boost::asio::ip::tcp::endpoint& endpoint);
    void announce_on_zeroconf();


    uint16_t ws_port() const {
        return m_ws_endpoint.port();
    }
    boost::uuids::uuid endpoint_uuid() const {
        return m_service_uid;
    }


    node_map nodes() const;
    std::shared_ptr<aseba_node> node_from_id(const node_id&) const;

    std::shared_ptr<mobsya::group> group_from_id(const node_id&) const;

    void register_endpoint(std::shared_ptr<aseba_endpoint> ep);
    void unregister_expired_endpoints();

    void disconnect_all_wireless_endpoints();

private:
    void remove_duplicated_node(const std::shared_ptr<aseba_node>& node);

    void save_group_affiliation(const aseba_node& node);
    void restore_group_affiliation(const aseba_node& node);

    void do_announce_on_zeroconf();
    void on_announce_complete(const boost::system::error_code&);
    aware::contact::property_map_type build_discovery_properties() const;

    node_map::const_iterator find(const std::shared_ptr<aseba_node>& node) const;
    node_map::const_iterator find_from_native_id(aseba_node::node_id_t id) const;
    boost::uuids::uuid m_service_uid;

    node_map m_aseba_nodes;
    // Listing the endpoints is useful to be able to configure
    // Thymio 2 dongles
    std::vector<std::weak_ptr<aseba_endpoint>> m_endpoints;

    //
    struct last_known_node_group {
        std::shared_ptr<mobsya::group> group;
        std::chrono::system_clock::time_point last_seen = std::chrono::system_clock::now();
    };
    std::map<node_id, last_known_node_group> m_ghost_groups;

    aware::announce_socket m_discovery_socket;
    aware::contact m_nodes_service_desc;
    // Endpoint of the WebSocket - So we can expose the port on zeroconf
    boost::asio::ip::tcp::endpoint m_ws_endpoint;

    boost::signals2::signal<void(std::shared_ptr<aseba_node>, node_id, aseba_node::status)>
        m_node_status_changed_signal;
    friend class node_status_monitor;
};


class node_status_monitor {
public:
    virtual ~node_status_monitor();
    void disconnect() {
        m_connection.disconnect();
    }
    virtual void node_changed(std::shared_ptr<aseba_node>, const aseba_node_registery::node_id&,
                              aseba_node::status) = 0;

protected:
    void start_node_monitoring(aseba_node_registery& registery) {
        m_connection = registery.m_node_status_changed_signal.connect(
            boost::bind(&node_status_monitor::node_changed, this, boost::placeholders::_1, boost::placeholders::_2,
                        boost::placeholders::_3));
    }

private:
    boost::signals2::scoped_connection m_connection;
};


}  // namespace mobsya
