#include "thymio-vpl2.h"
#include "aseba.h"
#include "simulator.h"
#include <QtQml>

void thymioVPL2Init() {
    qmlRegisterType<AsebaClient>("Aseba", 1, 0, "AsebaClient");
    qmlRegisterType<AsebaClient>("Aseba", 1, 0, "AsebaNode");
    qmlRegisterType<Simulator>("Simulator", 1, 0, "Simulator");
}
