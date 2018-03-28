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
    std::vector<ThymioInfo> getThymios() override;
    std::unique_ptr<QIODevice> openConnection(const ThymioInfo& thymio) override;

private:
    void updateService(QZeroConfService service);
    void onServiceAdded(QZeroConfService);
    void onServiceUpdated(QZeroConfService);
    void onServiceRemoved(QZeroConfService);
    QZeroConf* m_register;
};

}    // namespace mobsya
