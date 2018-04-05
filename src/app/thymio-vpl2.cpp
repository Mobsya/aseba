#include "thymio-vpl2.h"
#include "aseba.h"
#include <QtQml>
#include "thymio/ThymioListModel.h"

Q_DECLARE_METATYPE(AsebaClient::ThymioModel*);

void thymioVPL2Init() {

    qmlRegisterUncreatableType<AsebaClient::ThymioModel>("Thymio", 1, 0, "Model", "");
    qRegisterMetaType<AsebaClient::ThymioModel*>("Model");

    qmlRegisterType<AsebaClient>("Thymio", 1, 0, "AsebaClient");
    qmlRegisterType<AsebaClient>("Thymio", 1, 0, "AsebaNode");
}
