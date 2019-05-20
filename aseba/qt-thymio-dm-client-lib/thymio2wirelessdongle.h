#pragma once

#include <QObject>
#include <QUuid>
#include <QMap>
#include <QSet>
#include "request.h"

namespace mobsya {
class ThymioDeviceManagerClientEndpoint;
struct Thymio2WirelessDongle {
    QUuid uuid;
    uint16_t networkId;
};

class Thymio2WirelessDonglesManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QStringList dongles READ qml_dongles NOTIFY donglesChanged)

public:
    Thymio2WirelessDonglesManager(ThymioDeviceManagerClientEndpoint* parent);
    Q_INVOKABLE mobsya::Thymio2WirelessDongleInfoRequest dongleInfo(const QUuid& uuid);
    Q_INVOKABLE mobsya::Thymio2WirelessDongleInfoRequest pairThymio2Wireless(const QUuid& dongleId, const QUuid& nodeId,
                                                                             quint16 networkId, quint8 channel);

Q_SIGNALS:
    void donglesChanged();

private:
    ThymioDeviceManagerClientEndpoint* m_ep;
    friend class ThymioDeviceManagerClientEndpoint;
    void clear();
    void updateDongle(const QUuid& id);
    QStringList qml_dongles() const;
    QSet<QUuid> m_dongles;
};

}  // namespace mobsya

Q_DECLARE_METATYPE(mobsya::Thymio2WirelessDonglesManager*)