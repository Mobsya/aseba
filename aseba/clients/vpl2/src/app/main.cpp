#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QTranslator>
#include "thymio-vpl2.h"
#include "liveqmlreloadingengine.h"

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

#if 0  // defined(QT_DEBUG) && !defined(Q_OS_ANDROID)
    LiveQmlReloadingEngine engine;
    engine.load(QML_ROOT_FILE);
#else
    QQmlApplicationEngine engine;
    QQuickView v;
    v.setResizeMode(QQuickView::SizeRootObjectToView);
    v.setSource(QUrl(QStringLiteral(QML_ROOT_FILE)));
    v.setMinimumSize({800, 600});
    v.show();
#endif

    return app.exec();
}
