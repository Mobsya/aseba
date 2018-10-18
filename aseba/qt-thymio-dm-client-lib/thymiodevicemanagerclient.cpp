#include "thymiodevicemanagerclient.h"
#include "thymiodevicemanagerclientendpoint.h"
#include "qflatbuffers.h"
#include <QDebug>
#include <QTcpSocket>

namespace mobsya {


QUuid service_id(const QZeroConfService& service) {
    return service.txt().value("uuid");
}

ThymioDeviceManagerClient::ThymioDeviceManagerClient(QObject* parent)
    : QObject(parent), m_register(new QZeroConf(this)) {

    connect(m_register, &QZeroConf::serviceAdded, this, &ThymioDeviceManagerClient::onServiceAdded);
    connect(m_register, &QZeroConf::serviceUpdated, this, &ThymioDeviceManagerClient::onServiceAdded);
    connect(m_register, &QZeroConf::serviceRemoved, this, &ThymioDeviceManagerClient::onServiceRemoved);
    m_register->startBrowser("_mobsya._tcp");
}


std::shared_ptr<ThymioNode> ThymioDeviceManagerClient::node(const QUuid& id) const {
    return m_nodes.value(id);
}

void ThymioDeviceManagerClient::onServiceAdded(QZeroConfService service) {
    QUuid id = service_id(service);
    if(id.isNull())
        return;

    std::shared_ptr<ThymioDeviceManagerClientEndpoint> endpoint = m_endpoints[id];

    if(!endpoint) {
        QTcpSocket* socket = new QTcpSocket;
        endpoint = std::make_shared<ThymioDeviceManagerClientEndpoint>(socket);
        socket->connectToHost(service.ip(), service.port());
        connect(endpoint.get(), &ThymioDeviceManagerClientEndpoint::disconnected, this,
                &ThymioDeviceManagerClient::onEndpointDisconnected);

        connect(endpoint.get(), &ThymioDeviceManagerClientEndpoint::nodeAdded, this,
                &ThymioDeviceManagerClient::nodeAdded);
        connect(endpoint.get(), &ThymioDeviceManagerClientEndpoint::nodeAdded, this,
                &ThymioDeviceManagerClient::onNodeAdded);
        connect(endpoint.get(), &ThymioDeviceManagerClientEndpoint::nodeModified, this,
                &ThymioDeviceManagerClient::nodeModified);
        connect(endpoint.get(), &ThymioDeviceManagerClientEndpoint::nodeRemoved, this,
                &ThymioDeviceManagerClient::nodeRemoved);
        connect(endpoint.get(), &ThymioDeviceManagerClientEndpoint::nodeRemoved, this,
                &ThymioDeviceManagerClient::onNodeRemoved);

        m_endpoints.insert(id, endpoint);

    } else if(endpoint->peerAddress() != service.ip()) {
        return;
    }

    const auto properties = service.txt();
    uint16_t port = properties.value("ws-port", 0).toUInt();
    endpoint->setWebSocketMatchingPort(port);
}

void ThymioDeviceManagerClient::onNodeAdded(std::shared_ptr<ThymioNode> node) {
    m_nodes.insert(node->uuid(), node);
}

void ThymioDeviceManagerClient::onNodeRemoved(std::shared_ptr<ThymioNode> node) {
    m_nodes.remove(node->uuid());
}

void ThymioDeviceManagerClient::onServiceRemoved(QZeroConfService service) {
    QUuid id = service_id(service);
    if(id.isNull())
        return;
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
