#include "thymio2wirelessdongle.h"

namespace mobsya {

Thymio2WirelessDonglesManager::Thymio2WirelessDonglesManager(QObject* parent) : QObject(parent) {}
uint16_t Thymio2WirelessDonglesManager::networkId(const QUuid& uuid) {
    return m_dongles.value(uuid).networkId;
}

QStringList Thymio2WirelessDonglesManager::qml_dongles() const {
    QStringList lst;
    for(auto&& d : m_dongles) {
        lst.append(d.uuid.toString());
    }
    return lst;
}
void Thymio2WirelessDonglesManager::clear() {
    m_dongles.clear();
    Q_EMIT donglesChanged();
}
void Thymio2WirelessDonglesManager::updateDongle(const QUuid& id, uint16_t networkId) {
    auto& d = m_dongles[id];
    d.networkId = networkId;
    d.uuid = id;
    Q_EMIT donglesChanged();
}

}  // namespace mobsya
