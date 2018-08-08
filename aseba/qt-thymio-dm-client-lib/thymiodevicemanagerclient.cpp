#include "thymiodevicemanagerclient.h"
#include "thymiodevicemanagerclientendpoint.h"
#include <QDebug>
#include <QTcpSocket>

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
        socket->connectToHost(service.host(), service.port());
        if(!socket->waitForConnected(2000)) {
            delete socket;
            return;
        }
        std::shared_ptr<ThymioDeviceManagerClientEndpoint> endpoint =
            std::make_shared<ThymioDeviceManagerClientEndpoint>(socket);

        connect(endpoint.get(), &ThymioDeviceManagerClientEndpoint::onMessage, this,
                &ThymioDeviceManagerClient::onMessage);
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

void ThymioDeviceManagerClient::onNodesChanged(const fb::NodesChanged& nc_msg) {
    fb::NodesChangedT _t;
    nc_msg.UnPackTo(&_t);
    std::vector<SimpleNode> changed_nodes;
    for(const auto& ptr : _t.nodes) {
        const fb::NodeT& node = *ptr;
        const std::vector<uint8_t>& bytes = node.node_id->id;
        if(bytes.size() != 16)
            continue;
        SimpleNode deserialized{
            // Sometimes Qt apis aren't that convenient...
            QUuid(*(reinterpret_cast<const uint32_t*>(bytes.data())),
                  *(reinterpret_cast<const uint16_t*>(bytes.data() + 4)),
                  *(reinterpret_cast<const uint16_t*>(bytes.data() + 6)),
                  *(reinterpret_cast<const uint8_t*>(bytes.data() + 8)),
                  *(reinterpret_cast<const uint8_t*>(bytes.data() + 9)),
                  *(reinterpret_cast<const uint8_t*>(bytes.data() + 10)),
                  *(reinterpret_cast<const uint8_t*>(bytes.data() + 11)),
                  *(reinterpret_cast<const uint8_t*>(bytes.data() + 12)),
                  *(reinterpret_cast<const uint8_t*>(bytes.data() + 13)),
                  *(reinterpret_cast<const uint8_t*>(bytes.data() + 14)),
                  *(reinterpret_cast<const uint8_t*>(bytes.data() + 15))),
            QString::fromStdString(node.name),
            static_cast<ThymioNode::Status>(node.status),
        };
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
        auto it = m_nodes.find(node.id);
        if(it == m_nodes.end()) {
            it = m_nodes.insert(node.id, std::make_shared<ThymioNode>(shared_endpoint, node.id, node.name));
            Q_EMIT nodeAdded(it.value());
        }
        (*it)->setName(node.name);
        (*it)->setStatus(node.status);

        if(node.status == ThymioNode::Status::disconnected) {
            auto node = it.value();
            m_nodes.erase(it);
            Q_EMIT nodeRemoved(node);
        }
    }
}


}  // namespace mobsya
