#pragma once
#include <QObject>
#include <QTcpSocket>
#include <aseba/flatbuffers/fb_message_ptr.h>
#include <QUrl>

namespace mobsya {
class ThymioNode;
class ThymioDeviceManagerClientEndpoint : public QObject {
    Q_OBJECT

public:
    using request_id = quint32;
    ThymioDeviceManagerClientEndpoint(QTcpSocket* socket, QObject* parent = nullptr);

public:
    QUrl websocketConnectionUrl() const;
    void setWebSocketMatchingPort(quint16 port);

    request_id renameNode(const ThymioNode& node, const QString& newName);
    request_id stopNode(const ThymioNode& node);

private Q_SLOTS:
    void onReadyRead();
    void onConnected();
    void write(const flatbuffers::DetachedBuffer& buffer);

Q_SIGNALS:
    void onMessage(const fb_message_ptr& msg);
    void disconnected();

private:
    request_id generate_request_id();

    static flatbuffers::Offset<fb::NodeId> serialize_uuid(flatbuffers::FlatBufferBuilder& fb, const QUuid& uuid);
    QTcpSocket* m_socket;
    quint32 m_message_size;
    quint16 m_ws_port = 0;
};

}  // namespace mobsya
