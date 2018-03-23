#pragma once
#include <aseba/common/msg/msg.h>
#include <QObject>
#include <QIODevice>

namespace mobsya {

class DeviceQtConnection : public QObject {
    Q_OBJECT
public:
    DeviceQtConnection(QIODevice* device, QObject* parent = nullptr);
    bool isOpen() const;


Q_SIGNALS:
    void messageReceived(std::shared_ptr<Aseba::Message> message);
public Q_SLOTS:
    void sendMessage(const Aseba::Message& message);

private Q_SLOTS:
    void onDataAvailable();

private:
    QIODevice* m_device;
    quint16 m_messageSize;
};

}    // namespace mobsya
