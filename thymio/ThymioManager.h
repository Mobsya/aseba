#pragma once
#include <QObject>
#include <QVector>
#include "ThymioProviderInfo.h"
#include "DeviceQtConnection.h"

namespace mobsya {

constexpr int EPFL_VENDOR_ID = 0x0617;

class AbstractDeviceProber : public QObject {
    Q_OBJECT
public:
    AbstractDeviceProber(QObject* parent = nullptr);
    virtual std::vector<ThymioProviderInfo> getThymios() = 0;
Q_SIGNALS:
    void availabilityChanged();

public:
    virtual std::unique_ptr<QIODevice> openConnection(const ThymioProviderInfo& thymio) = 0;
};

class ThymioManager : public QObject {
    Q_OBJECT
public:
    ThymioManager(QObject* parent = nullptr);
    std::unique_ptr<DeviceQtConnection> openConnection(const ThymioProviderInfo& info);
    ThymioProviderInfo first() const;
    bool hasDevice() const;
public Q_SLOTS:
    void scanDevices();

private:
    std::vector<ThymioProviderInfo> m_thymios;
    QVector<AbstractDeviceProber*> m_probes;
};

}    // namespace mobsya
