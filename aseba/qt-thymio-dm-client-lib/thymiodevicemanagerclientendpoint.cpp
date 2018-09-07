#include "thymiodevicemanagerclientendpoint.h"
#include <QDataStream>
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
    flatbuffers::FlatBufferBuilder builder;
    write(wrap_fb(builder, fb::CreateConnectionHandshake(builder, minProtocolVersion, protocolVersion)));
}


}  // namespace mobsya
