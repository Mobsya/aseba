#include "remoteconnectionrequest.h"
#include <QHostInfo>
#include <qt-thymio-dm-client-lib/thymiodevicemanagerclient.h>
#include <qt-thymio-dm-client-lib/thymiodevicemanagerclientendpoint.h>

namespace mobsya {

RemoteConnectionRequest::RemoteConnectionRequest(ThymioDeviceManagerClient* client, const QString& host, quint16 port,
                                                 QObject* parent)
    : QObject(parent), m_client(client), m_host(host), m_port(port) {}

RemoteConnectionRequest::~RemoteConnectionRequest() {
    qDebug() << "d";
    if(m_req != -1)
        QHostInfo::abortHostLookup(m_req);
}

void RemoteConnectionRequest::start() {

    Q_EMIT message(tr("Connecting..."));
    QTcpSocket* socket = new QTcpSocket();
    m_endpoint = std::shared_ptr<ThymioDeviceManagerClientEndpoint>(
        new ThymioDeviceManagerClientEndpoint(socket, m_host),
        [](ThymioDeviceManagerClientEndpoint* ep) { ep->deleteLater(); });

    qRegisterMetaType<QAbstractSocket::SocketState>();
    qRegisterMetaType<QAbstractSocket::SocketError>();

    connect(socket, qOverload<QAbstractSocket::SocketError>(&QTcpSocket::error), this,
            &RemoteConnectionRequest::onTcpError);
    connect(socket, &QTcpSocket::connected, this, &RemoteConnectionRequest::onConnected);
    connect(m_endpoint.get(), &ThymioDeviceManagerClientEndpoint::handshakeCompleted, this,
            &RemoteConnectionRequest::onHandshakeCompleted);
    socket->connectToHost(m_host, m_port);
}

void RemoteConnectionRequest::onTcpError(QAbstractSocket::SocketError) {
    qDebug() << m_endpoint->socket()->errorString();
    Q_EMIT error(m_endpoint->socket()->errorString());
    m_endpoint.reset();
}

void RemoteConnectionRequest::onConnected() {
    qDebug() << "connected";
    Q_EMIT message(tr("Handshake..."));
}

void RemoteConnectionRequest::onHandshakeCompleted(QUuid id) {
    qDebug() << "hs";
    if(m_client->registerEndpoint(id, m_endpoint)) {
        Q_EMIT done();
    } else {
        error(tr("You are already connected to this server"));
        m_endpoint.reset();
    }
}

}  // namespace mobsya
