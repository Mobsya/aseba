#include "thymiodevicemanagerclientendpoint.h"
#include <QDataStream>
#include <QFile>
#include "thymio-api.h"

namespace mobsya {

ThymioDeviceManagerClientEndpoint::ThymioDeviceManagerClientEndpoint(QTcpSocket* socket, QObject* parent)
    : QObject(parent), m_socket(socket), m_message_size(0) {

    connect(m_socket, &QTcpSocket::readyRead, this, &ThymioDeviceManagerClientEndpoint::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &ThymioDeviceManagerClientEndpoint::disconnected);
    connect(m_socket, &QTcpSocket::connected, this, &ThymioDeviceManagerClientEndpoint::onConnected);
    // connect(m_socket, &QTcpSocket::onerror, this, &ThymioDeviceManagerClientEndpoint::disconnected);
}

void ThymioDeviceManagerClientEndpoint::write(const flatbuffers::DetachedBuffer& buffer) {
    uint32_t size = buffer.size();
    m_socket->write(reinterpret_cast<const char*>(&size), sizeof(size));
    m_socket->write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
}

void ThymioDeviceManagerClientEndpoint::setWebSocketMatchingPort(quint16 port) {
    m_ws_port = port;
}

void ThymioDeviceManagerClientEndpoint::setTokenFilePath(QString filePath) {
    m_token_file_path = filePath;
}

QUrl ThymioDeviceManagerClientEndpoint::websocketConnectionUrl() const {
    QUrl u;
    if(!m_ws_port)
        return {};
    u.setScheme("ws");
    u.setHost(m_socket->peerAddress().toString());
    u.setPort(m_ws_port);
    return u;
}

void ThymioDeviceManagerClientEndpoint::onReadyRead() {
    while(true) {
        if(m_message_size == 0) {
            if(m_socket->bytesAvailable() < 4)
                return;
            char data[4];
            m_socket->read(data, 4);
            m_message_size = *reinterpret_cast<quint32*>(data);
        }
        if(m_socket->bytesAvailable() < m_message_size)
            return;
        std::vector<uint8_t> data;
        data.resize(m_message_size);
        auto s = m_socket->read(reinterpret_cast<char*>(data.data()), m_message_size);
        Q_ASSERT(s == m_message_size);
        m_message_size = 0;
        Q_EMIT onMessage(fb_message_ptr(std::move(data)));
    }
}

void ThymioDeviceManagerClientEndpoint::onConnected() {
    QByteArray token;
    if(!m_token_file_path.isEmpty()) {
        QFile f(m_token_file_path);
        if(f.open(QIODevice::ReadOnly)) {
            token = f.readAll();
            qDebug() << token.toHex();
        }
    }

    flatbuffers::FlatBufferBuilder builder;
    flatbuffers::Offset<flatbuffers::Vector<uint8_t>> offset;
    if(!token.isEmpty()) {
        offset = builder.CreateVector<uint8_t>(reinterpret_cast<uint8_t*>(token.data()), token.size());
    }
    fb::ConnectionHandshakeBuilder hsb(builder);
    hsb.add_token(offset);
    hsb.add_protocolVersion(protocolVersion);
    hsb.add_minProtocolVersion(minProtocolVersion);
    write(wrap_fb(builder, hsb.Finish()));
}


}  // namespace mobsya
