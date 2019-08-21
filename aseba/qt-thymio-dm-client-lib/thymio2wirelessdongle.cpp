#include "thymio2wirelessdongle.h"
#include "thymiodevicemanagerclientendpoint.h"

namespace mobsya {

Thymio2WirelessDonglesManager::Thymio2WirelessDonglesManager(ThymioDeviceManagerClientEndpoint* parent)
    : QObject(parent), m_ep(parent) {}


Thymio2WirelessDonglePairingRequest Thymio2WirelessDonglesManager::pairThymio2Wireless(const QUuid& dongleId,
                                                                                       const QUuid& nodeId,
                                                                                       quint16 networkId,
                                                                                       quint8 channel) {
    return m_ep->pairThymio2Wireless(dongleId, nodeId, networkId, channel);
}

QVariantList Thymio2WirelessDonglesManager::dongles() const {
    QVariantList v;
    for(auto&& e : m_dongles.values()) {
        v.append(QVariant::fromValue(e));
    }
    return v;
}

void Thymio2WirelessDonglesManager::clear() {
    m_dongles.clear();
    Q_EMIT donglesChanged();
}
void Thymio2WirelessDonglesManager::updateDongle(const Thymio2WirelessDongle& dongle) {
    m_dongles.insert(dongle.uuid, dongle);
    Q_EMIT donglesChanged();
}

}  // namespace mobsya
