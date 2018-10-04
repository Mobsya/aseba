#include "thymionode.h"
#include "thymiodevicemanagerclientendpoint.h"
#include "qflatbuffers.h"

namespace mobsya {

ThymioNode::ThymioNode(std::shared_ptr<ThymioDeviceManagerClientEndpoint> endpoint, const QUuid& uuid,
                       const QString& name, NodeType type, QObject* parent)
    : QObject(parent), m_endpoint(endpoint), m_uuid(uuid), m_name(name), m_status(Status::Disconnected), m_type(type) {

    connect(this, &ThymioNode::nameChanged, this, &ThymioNode::modified);
    connect(this, &ThymioNode::statusChanged, this, &ThymioNode::modified);
    connect(this, &ThymioNode::capabilitiesChanged, this, &ThymioNode::modified);
}


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

ThymioNode::NodeCapabilities ThymioNode::capabilities() {
    return m_capabilities;
}

void ThymioNode::setCapabilities(const NodeCapabilities& capabilities) {
    if(m_capabilities != capabilities) {
        m_capabilities = capabilities;
        Q_EMIT capabilitiesChanged();
    }
}

void ThymioNode::setName(const QString& name) {
    if(m_name != name) {
        m_name = name;
        Q_EMIT nameChanged();
    }
}

ThymioDeviceManagerClientEndpoint::request_id ThymioNode::rename(const QString& newName) {
    if(newName != m_name) {
        Q_EMIT nameChanged();
        return m_endpoint->renameNode(*this, newName);
    }
    return 0;
}

ThymioDeviceManagerClientEndpoint::request_id ThymioNode::stop() {
    return m_endpoint->stopNode(*this);
}

void ThymioNode::setStatus(const Status& status) {
    if(m_status != status) {
        m_status = status;
        Q_EMIT statusChanged();
    }
}

}  // namespace mobsya
