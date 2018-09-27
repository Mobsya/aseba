#include "thymiodevicemanagerclient.h"
#include "thymiodevicemanagerclientendpoint.h"
#include <QDebug>
#include <QTcpSocket>
#include <QtEndian>

namespace mobsya {


QUuid service_id(const QZeroConfService& service) {
    return service.txt().value("uuid");
}

ThymioDeviceManagerClient::ThymioDeviceManagerClient(QObject* parent)
    : QObject(parent), m_register(new QZeroConf(this)) {

    connect(m_register, &QZeroConf::serviceAdded, this, &ThymioDeviceManagerClient::onServiceAdded);
    connect(m_register, &QZeroConf::serviceRemoved, this, &ThymioDeviceManagerClient::onServiceRemoved);
    m_register->startBrowser("_mobsya._tcp");
}


void ThymioDeviceManagerClient::onServiceAdded(QZeroConfService service) {
    QUuid id = service_id(service);
    if(id.isNull())
        return;

    if(m_endpoints.contains(id))
        return;

    else {
        QTcpSocket* socket = new QTcpSocket;
        std::shared_ptr<ThymioDeviceManagerClientEndpoint> endpoint =
            std::make_shared<ThymioDeviceManagerClientEndpoint>(socket);
        socket->connectToHost(service.ip(), service.port());
        const auto properties = service.txt();
        uint16_t port = properties.value("ws-port", 0).toUInt();
        endpoint->setWebSocketMatchingPort(port);

        connect(endpoint.get(), &ThymioDeviceManagerClientEndpoint::onMessage, this,
                &ThymioDeviceManagerClient::onMessage);
        connect(endpoint.get(), &ThymioDeviceManagerClientEndpoint::disconnected, this,
                &ThymioDeviceManagerClient::onEndpointDisconnected);
        m_endpoints.insert(id, endpoint);
    }
}

void ThymioDeviceManagerClient::onServiceRemoved(QZeroConfService service) {
    QUuid id = service_id(service);
    if(id.isNull())
        return;
}

void ThymioDeviceManagerClient::onMessage(const fb_message_ptr& msg) {
    if(const fb::NodesChanged* nc_msg = msg.as<mobsya::fb::NodesChanged>()) {
        qDebug() << "Nodes changed";
        onNodesChanged(*nc_msg);
    }
}

template <typename T>
T to_uuid_part(const unsigned char* c) {
    return qToBigEndian(*(reinterpret_cast<const T*>(c)));
}

void ThymioDeviceManagerClient::onNodesChanged(const fb::NodesChanged& nc_msg) {
    fb::NodesChangedT _t;
    nc_msg.UnPackTo(&_t);
    std::vector<SimpleNode> changed_nodes;
    for(const auto& ptr : _t.nodes) {
        const fb::NodeT& node = *ptr;
        const std::vector<uint8_t>& bytes = node.node_id->id;
        if(bytes.size() != 16)
            continue;

        ThymioNode::NodeCapabilities caps;
        if(node.capabilities & uint64_t(fb::NodeCapability::ForceResetAndStop))
            caps |= ThymioNode::NodeCapability::ForceResetAndStop;
        if(node.capabilities & uint64_t(fb::NodeCapability::Rename))
            caps |= ThymioNode::NodeCapability::Rename;

        SimpleNode deserialized{
            // Sometimes Qt apis aren't that convenient...
            QUuid(to_uuid_part<uint32_t>(bytes.data()), to_uuid_part<uint16_t>(bytes.data() + 4),
                  to_uuid_part<uint16_t>(bytes.data() + 6), to_uuid_part<uint8_t>(bytes.data() + 8),
                  to_uuid_part<uint8_t>(bytes.data() + 9), to_uuid_part<uint8_t>(bytes.data() + 10),
                  to_uuid_part<uint8_t>(bytes.data() + 11), to_uuid_part<uint8_t>(bytes.data() + 12),
                  to_uuid_part<uint8_t>(bytes.data() + 13), to_uuid_part<uint8_t>(bytes.data() + 14),
                  to_uuid_part<uint8_t>(bytes.data() + 15)),
            QString::fromStdString(node.name), static_cast<ThymioNode::Status>(node.status),
            static_cast<ThymioNode::NodeType>(node.type), caps};
        changed_nodes.emplace_back(std::move(deserialized));
    }
    onNodesChanged(changed_nodes);
}

void ThymioDeviceManagerClient::onNodesChanged(const std::vector<SimpleNode>& nodes) {
    auto endpoint = qobject_cast<ThymioDeviceManagerClientEndpoint*>(sender());
    auto it = std::find_if(m_endpoints.begin(), m_endpoints.end(),
                           [endpoint](const auto& ep) { return ep.get() == endpoint; });
    if(it == std::end(m_endpoints))
        return;

    const auto shared_endpoint = *it;

    for(const auto& node : nodes) {
        bool added = false;
        auto it = m_nodes.find(node.id);
        if(it == m_nodes.end()) {
            it = m_nodes.insert(node.id,
                                std::make_shared<ThymioNode>(shared_endpoint, node.id, node.name, node.type, this));
            added = true;
        }

        (*it)->setName(node.name);
        (*it)->setStatus(node.status);
        (*it)->setCapabilities(node.capabilities);

        Q_EMIT added ? nodeAdded(it.value()) : nodeModified(it.value());

        if(node.status == ThymioNode::Status::Disconnected) {
            auto node = it.value();
            m_nodes.erase(it);
            Q_EMIT nodeRemoved(node);
        }
    }
}

void ThymioDeviceManagerClient::onEndpointDisconnected() {
    auto endpoint = qobject_cast<ThymioDeviceManagerClientEndpoint*>(sender());
    auto it = std::find_if(m_endpoints.begin(), m_endpoints.end(),
                           [endpoint](const auto& ep) { return ep.get() == endpoint; });
    if(it == std::end(m_endpoints))
        return;
    const auto shared_endpoint = *it;

    auto node_it =
        std::remove_if(m_nodes.begin(), m_nodes.end(), [shared_endpoint](const std::shared_ptr<ThymioNode>& node) {
            return node->endpoint() == shared_endpoint;
        });
    while(node_it != m_nodes.end()) {
        auto node = node_it.value();
        node_it = m_nodes.erase(node_it);
        Q_EMIT nodeRemoved(node);
    }
    m_endpoints.erase(it);
}


}  // namespace mobsya
