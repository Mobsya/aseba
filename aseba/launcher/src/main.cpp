#include <QApplication>
#include <QFontDatabase>
#include <QDir>
#include <QDebug>
#include <QQmlApplicationEngine>
#include <QSurfaceFormat>
#include <QtQuickWidgets/QQuickWidget>
#ifdef QT_QML_DEBUG
#    include <QQmlDebuggingEnabler>
#endif
#include <aseba/qt-thymio-dm-client-lib/thymio-api.h>
#include <QtSingleApplication>
#include "launcher.h"
#include "tdmsupervisor.h"

int main(int argc, char** argv) {

#ifdef QT_QML_DEBUG
    QQmlDebuggingEnabler enabler;
#endif

    // Ensure a single instance
    QtSingleApplication app(argc, argv);
    if(app.sendMessage("ACTIVATE")) {
        qWarning("Already launched, exiting");
        return 0;
    }

    mobsya::register_qml_types();

    for(QString& file : QDir(":/fonts").entryList(QDir::Files)) {
        QString path = ":/fonts/" + file;
        QFontDatabase::addApplicationFont(path);
    }

    QSurfaceFormat format;
    format.setSamples(16);
    QSurfaceFormat::setDefaultFormat(format);

    mobsya::Launcher launcher;
    mobsya::TDMSupervisor supervisor(launcher);
    supervisor.startLocalTDM();

    mobsya::ThymioDeviceManagerClient client;
    mobsya::ThymioDevicesModel model(client);

    QQuickWidget w;
    w.rootContext()->setContextProperty("Utils", &launcher);
    w.rootContext()->setContextProperty("thymios", &model);
    w.setSource(QUrl(QStringLiteral("qrc:/qml/main.qml")));
    w.setResizeMode(QQuickWidget::SizeRootObjectToView);
    w.setFormat(format);
    w.setMinimumSize(1024, 640);
    w.showNormal();

    app.setActivationWindow(&w, true);

    return app.exec();
}
