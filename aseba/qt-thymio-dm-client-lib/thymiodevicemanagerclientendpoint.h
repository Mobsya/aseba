#pragma once
#include <QObject>
#include <QTcpSocket>
#include <aseba/flatbuffers/fb_message_ptr.h>
#include <QUrl>

namespace mobsya {
class ThymioDeviceManagerClientEndpoint : public QObject {
    Q_OBJECT

public:
    ThymioDeviceManagerClientEndpoint(QTcpSocket* socket, QObject* parent = nullptr);

public:
    QUrl websocketConnectionUrl() const;
    void setWebSocketMatchingPort(quint16 port);
    void setTokenFilePath(QString filePath);

private Q_SLOTS:
    void onReadyRead();
    void onConnected();
    void write(const flatbuffers::DetachedBuffer& buffer);

Q_SIGNALS:
    void onMessage(const fb_message_ptr& msg);
    void disconnected();

private:
    QTcpSocket* m_socket;
    quint32 m_message_size;
    quint16 m_ws_port = 0;
    QString m_token_file_path;
};

}  // namespace mobsya
