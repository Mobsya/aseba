#pragma once
#include <QObject>
#include <QHostInfo>
#include <memory>
#include <QUuid>

namespace mobsya {
class ThymioDeviceManagerClientEndpoint;
class ThymioDeviceManagerClient;
class RemoteConnectionRequest : public QObject {
    Q_OBJECT
public:
    RemoteConnectionRequest(ThymioDeviceManagerClient* client, const QString& host, quint16 port, QObject* parent = 0);
    ~RemoteConnectionRequest();

Q_SIGNALS:
    void error(QString message);
    void message(QString message);
    void done();

public Q_SLOTS:
    void start();

private Q_SLOTS:
    void onTcpError(QAbstractSocket::SocketError);
    void onConnected();
    void onHandshakeCompleted(QUuid id);

private:
    void onLookup(QHostInfo);
    ThymioDeviceManagerClient* m_client;
    QString m_host;
    quint16 m_port;
    int m_req = -1;
    std::shared_ptr<ThymioDeviceManagerClientEndpoint> m_endpoint;
};

}  // namespace mobsya