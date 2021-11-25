#include "thymiodevicemanagerclient.h"
#include "thymiodevicemanagerclientendpoint.h"
#include "qflatbuffers.h"
#include <QDebug>
#include <QTcpSocket>
#include <aseba/qt-thymio-dm-client-lib/remoteconnectionrequest.h>

namespace mobsya {


QUuid service_id(const QZeroConfService& service) {
    return service.txt().value("uuid");
}

ThymioDeviceManagerClient::ThymioDeviceManagerClient(QObject* parent)
    : QObject(parent), m_register(new QZeroConf(this)) {

    connect(m_register, &QZeroConf::error, this, &ThymioDeviceManagerClient::zeroconfBrowserStatusChanged);
    connect(m_register, &QZeroConf::serviceAdded, this, &ThymioDeviceManagerClient::onServiceAdded);
    connect(m_register, &QZeroConf::serviceUpdated, this, &ThymioDeviceManagerClient::onServiceAdded);
    connect(m_register, &QZeroConf::serviceRemoved, this, &ThymioDeviceManagerClient::onServiceRemoved);
    m_register->startBrowser("_mobsya._tcp");
}


std::shared_ptr<ThymioNode> ThymioDeviceManagerClient::node(const QUuid& id) const {
    return m_nodes.value(id);
}

bool ThymioDeviceManagerClient::isZeroconfBrowserConnected() const {
    return m_register->browserExists();
}

bool ThymioDeviceManagerClient::hasEndpoint(QUuid uuid) const {
    auto it = m_endpoints.find(uuid);
    if(it == m_endpoints.end()) {
        return false;
    }
    return it->operator bool();
}

void ThymioDeviceManagerClient::connectToRemoteUrlEndpoint(QUrl endpoint, QByteArray password) {
    QUrl url(endpoint);
    QString host = url.host();
    quint16 port = url.port();
    connectToRemoteEndpoint(host, port, password);
}

void ThymioDeviceManagerClient::connectToRemoteEndpoint(QString host, quint16 port, QByteArray password) {
    auto c = new mobsya::RemoteConnectionRequest(this, host, port, password, this);
    QObject::connect(c, &mobsya::RemoteConnectionRequest::done, &mobsya::RemoteConnectionRequest::deleteLater);
    QObject::connect(c, &mobsya::RemoteConnectionRequest::error, &mobsya::RemoteConnectionRequest::deleteLater);
    QTimer::singleShot(0, c, &mobsya::RemoteConnectionRequest::start);
}

void ThymioDeviceManagerClient::doRegisterEndpoint(std::shared_ptr<ThymioDeviceManagerClientEndpoint>& endpoint) {
    connect(endpoint.get(), &ThymioDeviceManagerClientEndpoint::disconnected, this,
            &ThymioDeviceManagerClient::onEndpointDisconnected);
    connect(endpoint.get(), &ThymioDeviceManagerClientEndpoint::nodeAdded, this,
            &ThymioDeviceManagerClient::onNodeAdded);
    connect(endpoint.get(), &ThymioDeviceManagerClientEndpoint::nodeModified, this,
            &ThymioDeviceManagerClient::nodeModified);
    connect(endpoint.get(), &ThymioDeviceManagerClientEndpoint::nodeRemoved, this,
            &ThymioDeviceManagerClient::onNodeRemoved);
}

bool ThymioDeviceManagerClient::registerEndpoint(QUuid id,
                                                 std::shared_ptr<ThymioDeviceManagerClientEndpoint> endpoint) {
    if(hasEndpoint(id))
        return false;
    doRegisterEndpoint(endpoint);
    m_endpoints.insert(id, endpoint);

    if(endpoint->isLocalhostPeer()) {
        Q_EMIT localEndpointChanged();
    }

    return true;
}

void ThymioDeviceManagerClient::onServiceAdded(QZeroConfService service) {
    qCDebug(zeroconf) << "ThymioDeviceManagerClient::onServiceAdded: " << service;
    QUuid id = service_id(service);
    qCDebug(zeroconf) << "ThymioDeviceManagerClient::onServiceAdded: id: " << id;
    if(id.isNull())
        return;

    auto endpoint = m_endpoints[id];

    if(!endpoint) {
        QTcpSocket* socket = new QTcpSocket;
        endpoint = std::shared_ptr<ThymioDeviceManagerClientEndpoint>(
            new ThymioDeviceManagerClientEndpoint(socket, service.host()),
            [](ThymioDeviceManagerClientEndpoint* ep) { ep->deleteLater(); });
        socket->connectToHost(service.ip(), service.port());
        doRegisterEndpoint(endpoint);
        m_endpoints.insert(id, endpoint);

    } else if(endpoint->peerAddress() != service.ip()) {
        return;
    }

    const auto properties = service.txt();
    uint16_t port = properties.value("ws-port", 0).toUInt();
    if(port != 0)
        endpoint->setWebSocketMatchingPort(port);
}

void ThymioDeviceManagerClient::onNodeAdded(std::shared_ptr<ThymioNode> node) {
    m_nodes.insert(node->uuid(), node);
    Q_EMIT nodeAdded(node);
}

void ThymioDeviceManagerClient::onNodeRemoved(std::shared_ptr<ThymioNode> node) {
    m_nodes.remove(node->uuid());
    Q_EMIT nodeRemoved(node);
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
    if(it != std::end(m_endpoints)) {
        const auto shared_endpoint = *it;
        for(auto node_it = m_nodes.begin(); node_it != m_nodes.end();) {
            if(!node_it.value() || node_it.value()->endpoint() == shared_endpoint) {
                const auto node = node_it.value();
                node_it = m_nodes.erase(node_it);
                if(node) {
                    Q_EMIT nodeRemoved(node);
                }
                continue;
            }
            ++node_it;
        }
        m_endpoints.erase(it);
    }
    if(endpoint && endpoint->isLocalhostPeer()) {
        Q_EMIT localPeerDisconnected();
        Q_EMIT localEndpointChanged();
    }
}

ThymioDeviceManagerClientEndpoint* ThymioDeviceManagerClient::qml_localEndpoint() const {
    auto it = std::find_if(m_endpoints.begin(), m_endpoints.end(),
                           [](const auto& ep) { return ep && ep->isLocalhostPeer(); });
    if(it == std::end(m_endpoints))
        return nullptr;
    return it->get();
}

void ThymioDeviceManagerClient::requestDeviceManagersShutdown() {
    for(auto&& ep : m_endpoints.values()) {
        if(ep && ep->isLocalhostPeer()) {
            ep->requestDeviceManagerShutdown();
            return;
        }
    }
    Q_EMIT localPeerDisconnected();
}

void ThymioDeviceManagerClient::restartBrowser() {
        m_register->startBrowser("_mobsya._tcp");
}

}  // namespace mobsya
