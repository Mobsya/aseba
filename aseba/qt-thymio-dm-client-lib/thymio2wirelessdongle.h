#pragma once

#include <QObject>
#include <QUuid>
#include <QMap>
#include <QSet>
#include "request.h"

namespace mobsya {
class ThymioDeviceManagerClientEndpoint;


class Thymio2WirelessDongle {
    Q_GADGET
    Q_PROPERTY(QUuid uuid MEMBER uuid)
    Q_PROPERTY(quint16 networkId MEMBER networkId)
    Q_PROPERTY(quint16 dongleId MEMBER dongleId)
    Q_PROPERTY(quint8 channel MEMBER channel)

public:
    QUuid uuid;
    quint16 networkId;
    quint16 dongleId;
    quint8 channel;
};


}  // namespace mobsya

Q_DECLARE_METATYPE(mobsya::Thymio2WirelessDongle)

namespace mobsya {

class Thymio2WirelessDonglesManager : public QObject {
    Q_OBJECT

public:
    Thymio2WirelessDonglesManager(ThymioDeviceManagerClientEndpoint* parent);

    Q_INVOKABLE QVariantList dongles() const;
    Q_INVOKABLE mobsya::Thymio2WirelessDonglePairingRequest
    pairThymio2Wireless(const QUuid& dongleId, const QUuid& nodeId, quint16 networkId, quint8 channel);

Q_SIGNALS:
    void donglesChanged();

private:
    ThymioDeviceManagerClientEndpoint* m_ep;
    friend class ThymioDeviceManagerClientEndpoint;
    void clear();
    void updateDongle(const Thymio2WirelessDongle& dongle);

    QMap<QUuid, Thymio2WirelessDongle> m_dongles;
};

}  // namespace mobsya

Q_DECLARE_METATYPE(mobsya::Thymio2WirelessDonglesManager*)
Q_DECLARE_METATYPE(QList<mobsya::Thymio2WirelessDongle>)