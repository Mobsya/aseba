#pragma once
#include "thymiodevicemanagerclient.h"
#include "thymionode.h"
#include "thymiodevicesmodel.h"
#include "thymio2wirelessdongle.h"
#include "thymiodevicemanagerclientendpoint.h"
#include "remoteconnectionrequest.h"

#ifdef QT_QML_LIB
#    include "request.h"
#    include <QtQml/QtQml>
#endif
namespace mobsya {

constexpr const unsigned protocolVersion = 1;
constexpr const unsigned minProtocolVersion = 1;

#ifdef QT_QML_LIB
void inline register_qml_types() {
    qRegisterMetaType<mobsya::ThymioDeviceManagerClientEndpoint*>("ThymioDeviceManagerClientEndpoint*");
    qRegisterMetaType<mobsya::Thymio2WirelessDonglesManager*>("Thymio2WirelessDonglesManager*");
    qRegisterMetaType<QQmlListProperty<mobsya::ThymioNode>>("QQmlListProperty<ThymioNode>");
    qmlRegisterUncreatableType<mobsya::ThymioNode>("org.mobsya", 1, 0, "ThymioNode", "Enum");
    qRegisterMetaType<mobsya::Thymio2WirelessDongle>("Thymio2WirelessDongle");
    qRegisterMetaType<QList<mobsya::Thymio2WirelessDongle>>();
    qRegisterMetaType<mobsya::RemoteConnectionRequest*>("RemoteConnectionRequest*");
}
#endif

}  // namespace mobsya