#pragma once
#include "ThymioManager.h"
#include <qzeroconf.h>

class QZeroConf;

namespace mobsya {

QZeroConfService dns_service_for_provider(const ThymioProviderInfo& info);

class NetworkDeviceProber : public AbstractDeviceProber {
    Q_OBJECT
public:
    NetworkDeviceProber(QObject* parent = nullptr);
    ~NetworkDeviceProber();
    std::vector<ThymioProviderInfo> getDevices() override;
    std::shared_ptr<DeviceQtConnection> openConnection(const ThymioProviderInfo& thymio) override;

private:
    void onServiceAdded(QZeroConfService);
    void onServiceUpdated(QZeroConfService);
    void onServiceRemoved(QZeroConfService);
    QVector<QZeroConfService> m_services;
    QZeroConf* m_register;
};

}    // namespace mobsya
