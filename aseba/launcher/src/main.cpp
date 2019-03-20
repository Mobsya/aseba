#include <iostream>
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
#include <aseba/common/consts.h>
#include <aseba/qt-thymio-dm-client-lib/thymio-api.h>
#include <QtSingleApplication>
#include <qtwebengineglobal.h>
#include "launcher.h"
#include "tdmsupervisor.h"
#include "launcherwindow.h"

int main(int argc, char** argv) {

#ifdef Q_OS_WIN
    AttachConsole(ATTACH_PARENT_PROCESS);
    // The std stream are reopen using the console handle
    freopen("CONOUT$", "w", stdout);
    freopen("CONIN$", "r", stdin);
    freopen("CONERR$", "w", stderr);

    // make sure the console handles are returned by GetStdHandle
    HANDLE newOut = CreateFileW(L"CONOUT$", GENERIC_WRITE | GENERIC_READ, FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
    SetStdHandle(STD_OUTPUT_HANDLE, newOut);

    HANDLE newErr = CreateFileW(L"CONERR$", GENERIC_WRITE | GENERIC_READ, FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
    SetStdHandle(STD_ERROR_HANDLE, newErr);
    std::cout.clear();
    std::cerr.clear();
#endif  // Q_OS_WIN

#ifdef QT_QML_DEBUG
    QQmlDebuggingEnabler enabler;
#endif

    // Ensure a single instance
    QtSingleApplication app(argc, argv);
    if(app.sendMessage("ACTIVATE")) {
        qWarning("Already launched, exiting");
        return 0;
    }
    app.setQuitOnLastWindowClosed(false);

    mobsya::register_qml_types();

    for(QString& file : QDir(":/fonts").entryList(QDir::Files)) {
        QString path = ":/fonts/" + file;
        QFontDatabase::addApplicationFont(path);
    }

    mobsya::Launcher launcher;
    mobsya::TDMSupervisor supervisor(launcher);
    supervisor.startLocalTDM();

    mobsya::ThymioDeviceManagerClient client;
    mobsya::ThymioDevicesModel model(client);

    QApplication::setWindowIcon(QIcon(":/assets/thymio-launcher.ico"));
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setApplicationVersion(QStringLiteral("%1-%2").arg(ASEBA_VERSION).arg(ASEBA_REVISION));

    mobsya::LauncherWindow w;
    w.rootContext()->setContextProperty("Utils", &launcher);
    w.rootContext()->setContextProperty("thymios", &model);
    w.setSource(QUrl(QStringLiteral("qrc:/qml/main.qml")));
    w.setResizeMode(QQuickWidget::SizeRootObjectToView);
    w.setMinimumSize(1024, 640);
    w.showNormal();

    QObject::connect(&app, &QGuiApplication::lastWindowClosed, [&client]() {
        auto windows = qApp->allWindows();
        for(auto w : windows) {
            if(w->isVisible() && qobject_cast<QQuickWindow*>(w))
                return;
        }
        client.requestDeviceManagersShutdown();
        QTimer::singleShot(1000, qApp, &QCoreApplication::quit);
    });

    app.setActivationWindow(&w, true);
    app.setQuitOnLastWindowClosed(false);

    QtWebEngine::initialize();

    return app.exec();
}
