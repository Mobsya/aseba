#pragma once

#include <QObject>
#include <QUuid>
#include <QMap>

namespace mobsya {

struct Thymio2WirelessDongle {
    QUuid uuid;
    uint16_t networkId;
};

class Thymio2WirelessDonglesManager : public QObject {
    Q_OBJECT
    Q_PROPERTY(QStringList dongles READ qml_dongles NOTIFY donglesChanged)

public:
    Thymio2WirelessDonglesManager(QObject* parent = nullptr);
    Q_INVOKABLE quint16 networkId(const QUuid& uuid);

Q_SIGNALS:
    void donglesChanged();

private:
    friend class ThymioDeviceManagerClientEndpoint;
    void clear();
    void updateDongle(const QUuid& id, uint16_t networkId);
    QStringList qml_dongles() const;
    QMap<QUuid, Thymio2WirelessDongle> m_dongles;
};

}  // namespace mobsya

Q_DECLARE_METATYPE(mobsya::Thymio2WirelessDonglesManager*)