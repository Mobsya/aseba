#include <QtQml/QQmlExtensionPlugin>

#include "aseba.h"
#include <QtQml>
#include "thymio/ThymioListModel.h"


Q_DECLARE_METATYPE(AsebaClient::ThymioModel*);

class QExampleQmlPlugin : public QQmlExtensionPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    void registerTypes(const char* uri) {
        qmlRegisterUncreatableType<AsebaClient::ThymioModel>("Thymio", 1, 0, "Model", "");
        qRegisterMetaType<AsebaClient::ThymioModel*>("Model");
        qmlRegisterType<AsebaClient>("Thymio", 1, 0, "AsebaClient");
        qmlRegisterType<AsebaClient>("Thymio", 1, 0, "AsebaNode");
    }
};
