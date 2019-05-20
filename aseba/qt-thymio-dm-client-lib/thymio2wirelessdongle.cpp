#include "thymio2wirelessdongle.h"
#include "thymiodevicemanagerclientendpoint.h"

namespace mobsya {

Thymio2WirelessDonglesManager::Thymio2WirelessDonglesManager(ThymioDeviceManagerClientEndpoint* parent)
    : QObject(parent), m_ep(parent) {}


Thymio2WirelessDongleInfoRequest Thymio2WirelessDonglesManager::dongleInfo(const QUuid& uuid) {
    return m_ep->requestDongleInfo(uuid);
}

Thymio2WirelessDonglePairingRequest Thymio2WirelessDonglesManager::pairThymio2Wireless(const QUuid& dongleId,
                                                                                       const QUuid& nodeId,
                                                                                       quint16 networkId,
                                                                                       quint8 channel) {
    return m_ep->pairThymio2Wireless(dongleId, nodeId, networkId, channel);
}

QStringList Thymio2WirelessDonglesManager::qml_dongles() const {
    QStringList lst;
    for(auto&& d : m_dongles) {
        lst.append(d.toString());
    }
    return lst;
}

void Thymio2WirelessDonglesManager::clear() {
    m_dongles.clear();
    Q_EMIT donglesChanged();
}
void Thymio2WirelessDonglesManager::updateDongle(const QUuid& id) {
    m_dongles.insert(id);
    Q_EMIT donglesChanged();
}

}  // namespace mobsya
