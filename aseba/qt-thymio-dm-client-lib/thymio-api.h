#pragma once
#include "thymiodevicemanagerclient.h"
#include "thymionode.h"
#include "thymiodevicesmodel.h"
#include <QtQml/QtQml>

namespace mobsya {
void register_qml_types() {
    qmlRegisterUncreatableType<mobsya::ThymioNode>("org.mobsya", 1, 0, "ThymioNode", "Enum");
}
}  // namespace mobsya
