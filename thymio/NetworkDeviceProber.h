#pragma once
#include "ThymioManager.h"
#include "third_party/QtZeroConf/qzeroconf.h"

class QZeroConf;

namespace mobsya {
class NetworkDeviceProber : public AbstractDeviceProber {
    Q_OBJECT
public:
    NetworkDeviceProber(QObject* parent = nullptr);
    ~NetworkDeviceProber();
    std::vector<ThymioProviderInfo> getDevices() override;
    std::unique_ptr<QIODevice> openConnection(const ThymioProviderInfo& thymio) override;

private:
    void onServiceAdded(QZeroConfService);
    void onServiceUpdated(QZeroConfService);
    void onServiceRemoved(QZeroConfService);
    QVector<QZeroConfService> m_services;
    QZeroConf* m_register;
};

}    // namespace mobsya
