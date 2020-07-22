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
#ifndef MOBSYA_USE_WEBENGINE
	#include <qtwebviewfunctions.h>
#else
	#include <QtWebEngine>
#endif
#include "launcher.h"
#include "tdmsupervisor.h"
#include "launcherwindow.h"

int main(int argc, char** argv) {

#ifndef MOBSYA_USE_WEBENGINE
    qputenv("QT_WEBVIEW_PLUGIN", "native");
#endif

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

#ifndef MOBSYA_USE_WEBENGINE
    QtWebView::initialize();
#else
    QtWebEngine::initialize();
#endif

    app.setQuitOnLastWindowClosed(false);

    mobsya::register_qml_types();

    for(QString& file : QDir(":/fonts").entryList(QDir::Files)) {
        QString path = ":/fonts/" + file;
        QFontDatabase::addApplicationFont(path);
    }

    mobsya::ThymioDeviceManagerClient client;
    mobsya::Launcher launcher(&client);
    mobsya::TDMSupervisor supervisor(launcher);
    supervisor.startLocalTDM();
    supervisor.stopLocalTDM();
    client.requestDeviceManagersShutdown();
//     supervisor.startLocalTDM();
//     launcher.connectToServer(QString("127.0.0.1"),quint16(8596));

    mobsya::ThymioDevicesModel model(client);

    QApplication::setWindowIcon(QIcon(":/assets/thymio-launcher.ico"));
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);

    auto load_trads = [](const QString& name, const QString& dir) {
        QTranslator* translator = new QTranslator(qApp);
        qDebug() << QLocale().name() << name << dir;
        if(translator->load(QLocale(), name, {}, dir)) {
            qApp->installTranslator(translator);
        } else {
            qDebug() << "Didn't load translation" << name;
        }
    };
    load_trads("launcher_", ":/translations");
    load_trads("qtdeclarative_", QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    load_trads("qtbase_", QLibraryInfo::location(QLibraryInfo::TranslationsPath));
    load_trads("qtwebengine_", QLibraryInfo::location(QLibraryInfo::TranslationsPath));


    QApplication::setApplicationName(QObject::tr("Thymio Suite"));
    QApplication::setApplicationVersion(QStringLiteral("%1-%2").arg(ASEBA_VERSION).arg(ASEBA_REVISION));

    mobsya::LauncherWindow w;
    w.rootContext()->setContextProperty("Utils", &launcher);
    w.rootContext()->setContextProperty("thymios", &model);
    w.rootContext()->setContextProperty("client", &client);
    w.setSource(QUrl(QStringLiteral("qrc:/qml/main.qml")));
    w.setResizeMode(QQuickWidget::SizeRootObjectToView);    
#ifdef Q_OS_IOS
    w.showFullScreen();
#else
    w.setMinimumSize(1024, 640);
    w.showNormal();
#endif
    QObject::connect(&app, &QGuiApplication::lastWindowClosed, [&]() {
        auto windows = qApp->allWindows();
        for(auto w : windows) {
            if(w->isVisible() && qobject_cast<QQuickWindow*>(w))
                return;
        }
        client.requestDeviceManagersShutdown();
        QTimer::singleShot(1000, qApp, &QCoreApplication::quit);
        launcher.writeSettings();
    });

    app.setActivationWindow(&w, true);
    app.setQuitOnLastWindowClosed(false);
    
#ifdef Q_OS_IOS
    QObject::connect(&app, &QGuiApplication::applicationStateChanged, &launcher, &mobsya::Launcher::applicationStateChanged);
#endif
    return app.exec();
}
