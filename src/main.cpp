#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QTranslator>
#include "thymio-vpl2.h"


#ifdef QMLLIVE_ENABLED
#    include "livenodeengine.h"
#    include "remotereceiver.h"
#endif

int main(int argc, char* argv[]) {
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QGuiApplication app(argc, argv);
    app.setOrganizationName("Mobsya");
    app.setOrganizationDomain("mobsya.org");
    app.setApplicationName("Thymio VPL Mobile Preview");

    QTranslator qtTranslator;
    qtTranslator.load("thymio-vpl2_" + QLocale::system().name(), ":/thymio-vpl2/translations/");
    app.installTranslator(&qtTranslator);

    thymioVPL2Init();

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral(QML_ROOT_FILE)));


#ifdef QMLLIVE_ENABLED
    LiveNodeEngine node;

    // Let QmlLive know your runtime
    node.setQmlEngine(&engine);

    // Allow it to display QML components with non-QQuickWindow root object
    QQuickView fallbackView(&engine, nullptr);
    node.setFallbackView(&fallbackView);

    // Tell it where file updates should be stored relative to
    node.setWorkspace(app.applicationDirPath(),
                      LiveNodeEngine::AllowUpdates | LiveNodeEngine::UpdatesAsOverlay);

    // Listen to IPC call from remote QmlLive Bench
    RemoteReceiver receiver;
    receiver.registerNode(&node);
    receiver.listen(10234);
#endif

    return app.exec();
}
