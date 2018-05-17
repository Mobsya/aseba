#include "aseba_node_registery.h"
#include "log.h"

namespace mobsya {

void aseba_node_registery::add_node(std::shared_ptr<aseba_node> node) {
    mLogInfo("Adding node");
    auto it = find(node);
    if(it == std::end(m_aseba_node)) {
        it = m_aseba_node.insert({0, node}).first;
    }
}

void aseba_node_registery::remove_node(std::shared_ptr<aseba_node> node) {
    mLogInfo("Removing node");
    auto it = find(node);
    if(it != std::end(m_aseba_node)) {
        m_aseba_node.erase(it);
    }
}

void aseba_node_registery::set_node_status(std::shared_ptr<aseba_node> node, aseba_node::status) {
    mLogInfo("Changing node status");
}

auto aseba_node_registery::find(std::shared_ptr<aseba_node> node) const -> node_map::const_iterator {
    for(auto it = std::begin(m_aseba_node); it != std::end(m_aseba_node); ++it) {
        if(it->second.expired())
            continue;
        if(it->second.lock() == node)
            return it;
    }
    return std::end(m_aseba_node);
}

}  // namespace mobsya
