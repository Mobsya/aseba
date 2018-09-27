#include "thymiodevicemanagerclientendpoint.h"
#include <QDataStream>
#include <QtEndian>
#include <array>
#include <QRandomGenerator>
#include "qflatbuffers.h"
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
    m_socket->flush();
}

void ThymioDeviceManagerClientEndpoint::setWebSocketMatchingPort(quint16 port) {
    m_ws_port = port;
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
    flatbuffers::FlatBufferBuilder builder;
    fb::ConnectionHandshakeBuilder hsb(builder);
    hsb.add_protocolVersion(protocolVersion);
    hsb.add_minProtocolVersion(minProtocolVersion);
    write(wrap_fb(builder, hsb.Finish()));
}

flatbuffers::Offset<fb::NodeId> ThymioDeviceManagerClientEndpoint::serialize_uuid(flatbuffers::FlatBufferBuilder& fb,
                                                                                  const QUuid& uuid) {
    std::array<uint8_t, 16> data;
    *reinterpret_cast<uint32_t*>(data.data()) = qFromBigEndian(uuid.data1);
    *reinterpret_cast<uint16_t*>(data.data() + 4) = qFromBigEndian(uuid.data2);
    *reinterpret_cast<uint16_t*>(data.data() + 6) = qFromBigEndian(uuid.data3);
    memcpy(data.data() + 8, uuid.data4, 8);

    return mobsya::fb::CreateNodeId(fb, fb.CreateVector(data.data(), data.size()));
}

ThymioDeviceManagerClientEndpoint::request_id ThymioDeviceManagerClientEndpoint::renameNode(const ThymioNode& node,
                                                                                            const QString& newName) {

    auto requestId = generate_request_id();
    flatbuffers::FlatBufferBuilder builder;
    auto uuidOffset = serialize_uuid(builder, node.uuid());
    auto nameOffset = qfb::add_string(builder, newName);
    fb::RenameNodeBuilder rn(builder);
    rn.add_request_id(requestId);
    rn.add_node_id(uuidOffset);
    rn.add_new_name(nameOffset);
    write(wrap_fb(builder, rn.Finish()));
    return requestId;
}

ThymioDeviceManagerClientEndpoint::request_id ThymioDeviceManagerClientEndpoint::stopNode(const ThymioNode& node) {
    auto requestId = generate_request_id();
    flatbuffers::FlatBufferBuilder builder;
    auto uuidOffset = serialize_uuid(builder, node.uuid());
    write(wrap_fb(builder, fb::CreateStopNode(builder, requestId, uuidOffset)));
    return requestId;
}

ThymioDeviceManagerClientEndpoint::request_id ThymioDeviceManagerClientEndpoint::generate_request_id() {
    quint32 value = QRandomGenerator::global()->generate();
    return value;
}


}  // namespace mobsya
