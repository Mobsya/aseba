#include "aseba_node.h"
#include "aseba_endpoint.h"

namespace mobsya {

void aseba_node::on_message(const Aseba::Message& msg) {
    switch(msg.type) {
        case ASEBA_MESSAGE_NODE_PRESENT: {
            std::unique_lock<std::mutex> _(m_node_mutex);
            if(m_description.name.empty()) {
                auto locked = m_endpoint.lock();
                if(locked) {
                    locked->read_aseba_node_description(m_id);
                }
            }
            break;
        }
        default: break;
    }
}


void aseba_node::write_message(const Aseba::Message& msg) {}


void aseba_node::on_description(Aseba::TargetDescription description) {
    m_description = description;
}

void aseba_node::request_node_description() {}

}  // namespace mobsya
