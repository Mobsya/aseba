#include "DeviceQtConnection.h"
#include <QDataStream>
#include <iostream>

namespace mobsya {

DeviceQtConnection::DeviceQtConnection(QIODevice* device, QObject* parent)
    : QObject(parent)
    , m_device(device)
    , m_messageSize(0) {
    device->setParent(this);

    connect(m_device, &QIODevice::readyRead, this, &DeviceQtConnection::onDataAvailable);
}

void DeviceQtConnection::sendMessage(const Aseba::Message& message) {


    Aseba::Message::SerializationBuffer buffer;
    message.serializeSpecific(buffer);
    auto len = static_cast<uint16_t>(buffer.rawData.size());

    if(len > ASEBA_MAX_EVENT_ARG_SIZE) {
        std::cerr
            << "Message::serialize() : fatal error: message size exceeds maximum packet size.\n";
        std::cerr << "message payload size: " << len
                  << ", maximum packet payload size (excluding type): " << ASEBA_MAX_EVENT_ARG_SIZE
                  << ", message type: " << std::hex << std::showbase << message.type << std::dec
                  << std::noshowbase;
        std::cerr << std::endl;
        std::terminate();
    }

    QDataStream stream(m_device);
    stream.setByteOrder(QDataStream::LittleEndian);
    stream << (quint16)len;
    stream << (quint16)message.source;
    stream << (quint16)message.type;
    if(buffer.rawData.size())
        stream.writeRawData(reinterpret_cast<char*>(&buffer.rawData[0]), buffer.rawData.size());
}

bool DeviceQtConnection::isOpen() const {
    return m_device->isOpen();
}

void DeviceQtConnection::onDataAvailable() {
    QDataStream stream(m_device);
    stream.setByteOrder(QDataStream::LittleEndian);
    if(m_messageSize == 0) {
        if(m_device->bytesAvailable() < 6)
            return;
        stream >> m_messageSize;
    }
    const int available = m_device->bytesAvailable();
    if(available < m_messageSize + 4)
        return;
    quint16 source, type;
    stream >> source;
    stream >> type;

    // read content
    Aseba::Message::SerializationBuffer buffer;
    buffer.rawData.resize(m_messageSize);
    if(m_messageSize) {
        stream.readRawData(reinterpret_cast<char*>(buffer.rawData.data()), m_messageSize);
    }
    m_messageSize = 0;

    // deserialize message
    auto msg = std::shared_ptr<Aseba::Message>(Aseba::Message::create(source, type, buffer));
    msg->dump(std::wcerr);
    std::cerr << L"\n";
    Q_EMIT messageReceived(std::move(msg));
}


}    // namespace mobsya
