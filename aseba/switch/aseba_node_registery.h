#pragma once
#include <boost/asio/io_service.hpp>
#include <unordered_map>
#include "aseba_node.h"

namespace mobsya {

class aseba_node_registery : public boost::asio::detail::service_base<aseba_node_registery> {
public:
    using node_id = uint32_t;
    aseba_node_registery(boost::asio::io_context& io_context)
        : boost::asio::detail::service_base<aseba_node_registery>(io_context) {}

    void add_node(std::shared_ptr<aseba_node> node);
    void remove_node(std::shared_ptr<aseba_node> node);
    void set_node_status(std::shared_ptr<aseba_node> node, aseba_node::status);

private:
    using node_map = std::unordered_map<node_id, std::weak_ptr<aseba_node>>;

    node_map::const_iterator find(std::shared_ptr<aseba_node> node) const;
    node_map m_aseba_node;
};


}  // namespace mobsya
