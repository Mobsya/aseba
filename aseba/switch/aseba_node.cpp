#include "aseba_node.h"
#include "aseba_endpoint.h"
#include "aseba_node_registery.h"

namespace mobsya {

const std::string& aseba_node::status_to_string(aseba_node::status s) {
    static std::array<std::string, 4> strs = {"connected", "ready", "busy", "disconnected"};
    int i = int(s) - 1;
    if(i < 0 && i >= int(status::connected))
        return {};
    return strs[i];
}


aseba_node::aseba_node(boost::asio::io_context& ctx, node_id_t id, std::weak_ptr<mobsya::aseba_endpoint> endpoint)
    : m_id(id), m_status(status::disconnected), m_endpoint(std::move(endpoint)), m_io_ctx(ctx) {}

std::shared_ptr<aseba_node> aseba_node::create(boost::asio::io_context& ctx, node_id_t id,
                                               std::weak_ptr<mobsya::aseba_endpoint> endpoint) {
    auto node = std::make_shared<aseba_node>(ctx, id, std::move(endpoint));
    node->set_status(status::connected);
    return node;
}


void aseba_node::disconnect() {
    set_status(status::disconnected);
}

aseba_node::~aseba_node() {
    if(m_status.load() != status::disconnected) {
        mLogWarn("Node destroyed before being disconnected");
    }
}


void aseba_node::on_message(const Aseba::Message& msg) {
    /*switch(msg.type) {
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
    }*/
}


void aseba_node::set_status(status s) {
    m_status = s;
    auto& registery = boost::asio::use_service<aseba_node_registery>(m_io_ctx);
    switch(s) {
        case status::connected: registery.add_node(shared_from_this()); break;
        case status::disconnected: registery.remove_node(shared_from_this()); break;
        default: registery.set_node_status(shared_from_this(), s); break;
    }
}

void aseba_node::write_message(const Aseba::Message& msg) {}


void aseba_node::on_description(Aseba::TargetDescription description) {
    {
        std::unique_lock<std::mutex> _(m_node_mutex);
        m_description = description;
    }
    set_status(status::ready);
}

void aseba_node::request_node_description() {}

}  // namespace mobsya
