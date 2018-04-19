#pragma once
#include <aseba/common/msg/msg.h>
#include <QObject>
#include <QIODevice>
#include "ThymioProviderInfo.h"

namespace mobsya {

class DeviceQtConnection : public QObject {
    Q_OBJECT
public:
    DeviceQtConnection(const ThymioProviderInfo& provider, QIODevice* device,
                       QObject* parent = nullptr);
    bool isOpen() const;


Q_SIGNALS:
    void messageReceived(const ThymioProviderInfo& provider,
                         std::shared_ptr<Aseba::Message> message);
    void connectionStatusChanged();
public Q_SLOTS:
    void sendMessage(const Aseba::Message& message);

private Q_SLOTS:
    void onDataAvailable();

private:
    QIODevice* m_device;
    int m_messageSize;
    ThymioProviderInfo m_provider;
};

}    // namespace mobsya
