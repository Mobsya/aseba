#include "DeviceQtConnection.h"
#include <QDataStream>
#include <iostream>
#include <sstream>
#include <QDebug>
#include <QBuffer>

namespace mobsya {

DeviceQtConnection::DeviceQtConnection(const ThymioProviderInfo& provider, QIODevice* device, QObject* parent)
    : QObject(parent), m_device(device), m_messageSize(-1), m_provider(provider) {
    device->setParent(this);

    connect(m_device, &QIODevice::readyRead, this, &DeviceQtConnection::onDataAvailable, Qt::QueuedConnection);
}

void DeviceQtConnection::sendMessage(const Aseba::Message& message) {

    Aseba::Message::SerializationBuffer buffer;
    message.serializeSpecific(buffer);
    auto len = static_cast<uint16_t>(buffer.rawData.size());

    if(len > ASEBA_MAX_EVENT_ARG_SIZE) {
        std::cerr << "Message::serialize() : fatal error: message size exceeds maximum packet size.\n";
        std::cerr << "message payload size: " << len
                  << ", maximum packet payload size (excluding type): " << ASEBA_MAX_EVENT_ARG_SIZE
                  << ", message type: " << std::hex << std::showbase << message.type << std::dec << std::noshowbase;
        std::cerr << std::endl;
        std::terminate();
    }

    QByteArray data;
    data.reserve(len + 6);
    QDataStream stream(&data, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream << (quint16)len;
    stream << (quint16)message.source;
    stream << (quint16)message.type;
    if(buffer.rawData.size())
        stream.writeRawData(reinterpret_cast<char*>(&buffer.rawData[0]), buffer.rawData.size());
    m_device->write(data.data(), data.size());
}

bool DeviceQtConnection::isOpen() const {
    return m_device->isOpen();
}

void DeviceQtConnection::onDataAvailable() {
    QDataStream stream(m_device);
    stream.setByteOrder(QDataStream::LittleEndian);
    if(m_messageSize == -1) {
        if(m_device->bytesAvailable() < 2)
            return;
        quint16 len;
        stream >> len;
        m_messageSize = len;
    }
    const auto available = m_device->bytesAvailable();
    if(available < m_messageSize + 4)
        return;
    quint16 source, type;
    stream >> source;
    stream >> type;

    // read content
    Aseba::Message::SerializationBuffer buffer;
    buffer.rawData.resize(std::size_t(m_messageSize));
    if(m_messageSize) {
        stream.readRawData(reinterpret_cast<char*>(buffer.rawData.data()), m_messageSize);
    }
    m_messageSize = -1;

    // deserialize message
    auto msg = std::shared_ptr<Aseba::Message>(Aseba::Message::create(source, type, buffer));
    std::wstringstream s;
    msg->dump(s);
    qDebug() << QString::fromStdWString(s.str());
    Q_EMIT messageReceived(m_provider, std::move(msg));
    QMetaObject::invokeMethod(this, "onDataAvailable", Qt::QueuedConnection);
}


}  // namespace mobsya
