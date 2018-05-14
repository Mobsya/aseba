#pragma once
#include <memory>
#include <mutex>

#include "aseba/common/msg/msg.h"

namespace mobsya {
class aseba_endpoint;

class aseba_node {
public:
    using node_id_t = uint16_t;
    aseba_node(node_id_t id, std::weak_ptr<mobsya::aseba_endpoint> endpoint)
        : m_id(id), m_endpoint(std::move(endpoint)) {}

private:
    friend class aseba_endpoint;

    void on_message(const Aseba::Message& msg);
    void on_description(Aseba::TargetDescription description);
    void write_message(const Aseba::Message& msg);
    void request_node_description();

    node_id_t m_id;
    std::weak_ptr<mobsya::aseba_endpoint> m_endpoint;
    std::mutex m_node_mutex;
    Aseba::TargetDescription m_description;
};


}  // namespace mobsya
