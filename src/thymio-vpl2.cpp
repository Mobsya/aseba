#include "thymio-vpl2.h"
#include "aseba.h"
#include "simulator.h"
#include <QtQml>
#include "thymio/ThymioListModel.h"


Q_DECLARE_METATYPE(AsebaClient::ThymioModel*);
// Q_DECLARE_METATYPE(mobsya::ThymioListModel*);

void thymioVPL2Init() {
    qmlRegisterUncreatableType<AsebaClient::ThymioModel>("Aseba", 1, 0, "Model", "");
    qRegisterMetaType<AsebaClient::ThymioModel*>("Model");

    qmlRegisterType<AsebaClient>("Aseba", 1, 0, "AsebaClient");
    qmlRegisterType<AsebaClient>("Aseba", 1, 0, "AsebaNode");
    qmlRegisterType<Simulator>("Simulator", 1, 0, "Simulator");
}
