#pragma once
#include "thymiodevicemanagerclient.h"
#include "thymionode.h"
#include "thymiodevicesmodel.h"
#ifdef QT_QML_LIB
#    include "request.h"
#    include <QtQml/QtQml>
#endif
namespace mobsya {

constexpr const unsigned protocolVersion = 1;
constexpr const unsigned minProtocolVersion = 1;

#ifdef QT_QML_LIB
void register_qml_types() {
    qmlRegisterUncreatableType<mobsya::ThymioNode>("org.mobsya", 1, 0, "ThymioNode", "Enum");
}
#endif

}  // namespace mobsya
