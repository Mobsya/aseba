#pragma once
#include <QObject>
#include <QTcpSocket>
#include <aseba/flatbuffers/fb_message_ptr.h>
#include <QUrl>
#include "request.h"

namespace mobsya {
class ThymioNode;
class ThymioDeviceManagerClientEndpoint : public QObject {
    Q_OBJECT

public:
    using request_id = quint32;
    ThymioDeviceManagerClientEndpoint(QTcpSocket* socket, QObject* parent = nullptr);
    ~ThymioDeviceManagerClientEndpoint();

public:
    QHostAddress peerAddress() const;
    QUrl websocketConnectionUrl() const;
    void setWebSocketMatchingPort(quint16 port);

    Request renameNode(const ThymioNode& node, const QString& newName);
    Request stopNode(const ThymioNode& node);

private Q_SLOTS:
    void onReadyRead();
    void onConnected();
    void cancelAllRequests();
    void write(const flatbuffers::DetachedBuffer& buffer);
    void write(const tagged_detached_flatbuffer& buffer);

Q_SIGNALS:
    void onMessage(const fb_message_ptr& msg) const;
    void disconnected();

private:
    template <typename T>
    T prepare_request() {
        T r = T::make_request();
        m_pending_requests.insert(r.id(), r.get_ptr());
        return r;
    }

    void handleIncommingMessage(const fb_message_ptr& msg);
    detail::RequestDataBase::shared_ptr get_request(detail::RequestDataBase::request_id);


    static flatbuffers::Offset<fb::NodeId> serialize_uuid(flatbuffers::FlatBufferBuilder& fb, const QUuid& uuid);
    QTcpSocket* m_socket;
    quint32 m_message_size;
    quint16 m_ws_port = 0;
    QMap<detail::RequestDataBase::request_id, detail::RequestDataBase::shared_ptr> m_pending_requests;
};

}  // namespace mobsya
