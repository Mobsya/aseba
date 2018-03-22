#pragma once
#include <QObject>
#include <QVector>
#include "ThymioInfo.h"

namespace mobsya {

constexpr int EPFL_VENDOR_ID = 0x0617;

class AbstractDeviceProber : public QObject {
    Q_OBJECT
public:
    AbstractDeviceProber(QObject* parent = nullptr);
    virtual std::vector<ThymioInfo> getThymios() = 0;
Q_SIGNALS:
    void availabilityChanged();
};

class ThymioManager : public QObject {
    Q_OBJECT
public:
    ThymioManager(QObject* parent = nullptr);

public Q_SLOTS:
    void scanDevices();

private:
    std::vector<ThymioInfo> m_thymios;
    QVector<AbstractDeviceProber*> m_probes;
};

}    // namespace mobsya
