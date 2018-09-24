#include "thymionode.h"


namespace mobsya {

ThymioNode::ThymioNode(std::shared_ptr<ThymioDeviceManagerClientEndpoint> endpoint, const QUuid& uuid,
                       const QString& name, NodeType type)
    : m_endpoint(endpoint), m_uuid(uuid), m_name(name), m_status(Status::Disconnected), m_type(type) {}


QUuid ThymioNode::uuid() const {
    return m_uuid;
}

QString ThymioNode::name() const {
    return m_name;
}

ThymioNode::Status ThymioNode::status() const {
    return m_status;
}

ThymioNode::NodeType ThymioNode::type() const {
    return m_type;
}

QUrl ThymioNode::websocketEndpoint() const {
    return m_endpoint->websocketConnectionUrl();
}


void ThymioNode::setName(const QString& name) {
    if(m_name != name) {
        m_name = name;
        Q_EMIT nameChanged();
    }
}
void ThymioNode::setStatus(const Status& status) {
    if(m_status != status) {
        m_status = status;
        Q_EMIT statusChanged();
    }
}

}  // namespace mobsya
