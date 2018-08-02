#pragma once
#include <QObject>
#include <QTcpSocket>
#include <aseba/flatbuffers/fb_message_ptr.h>

namespace mobsya {

class ThymioDeviceManagerClientEndpoint : public QObject {
    Q_OBJECT

public:
    ThymioDeviceManagerClientEndpoint(QTcpSocket* socket, QObject* parent = nullptr);

private Q_SLOTS:
    void onReadyRead();

Q_SIGNALS:
    void onMessage(const fb_message_ptr& msg);

private:
    QTcpSocket* m_socket;
    quint32 m_message_size;
};

}  // namespace mobsya
