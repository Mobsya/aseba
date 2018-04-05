#pragma once
#include "ThymioManager.h"
#include "ThymioProviderInfo.h"
#include <vector>

namespace mobsya {


class AndroidSerialDeviceProber : public AbstractDeviceProber {
    Q_OBJECT
public:
    std::vector<ThymioProviderInfo> getDevices() override;
    ~AndroidSerialDeviceProber();
    static AndroidSerialDeviceProber* instance();
    std::unique_ptr<QIODevice> openConnection(const ThymioProviderInfo& info) override;

private:
    AndroidSerialDeviceProber(QObject* parent = nullptr);

public:
    // For java
    void onDeviceAvailabilityChanged();
};


}    // namespace mobsya
