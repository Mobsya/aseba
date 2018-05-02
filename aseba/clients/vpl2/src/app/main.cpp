#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QTranslator>
#include "thymio-vpl2.h"
#include "liveqmlreloadingengine.h"
#include <QDebug>


const char* const applicationName = "Thymio VPL Mobile Preview";

#ifdef ANDROID  // Set in my myapp.pro file for android builds
#    include <android/log.h>

void myMessageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg) {
    QString report = msg;
    if(context.file && !QString(context.file).isEmpty()) {
        report += " in file ";
        report += QString(context.file);
        report += " line ";
        report += QString::number(context.line);
    }
    if(context.function && !QString(context.function).isEmpty()) {
        report += +" function ";
        report += QString(context.function);
    }
    const char* const local = report.toLocal8Bit().constData();
    switch(type) {
        case QtDebugMsg: __android_log_write(ANDROID_LOG_DEBUG, applicationName, local); break;
        case QtInfoMsg: __android_log_write(ANDROID_LOG_INFO, applicationName, local); break;
        case QtWarningMsg: __android_log_write(ANDROID_LOG_WARN, applicationName, local); break;
        case QtCriticalMsg: __android_log_write(ANDROID_LOG_ERROR, applicationName, local); break;
        case QtFatalMsg:
        default: __android_log_write(ANDROID_LOG_FATAL, applicationName, local); abort();
    }
}
#endif

int main(int argc, char* argv[]) {

    QGuiApplication app(argc, argv);
    app.setApplicationName(applicationName);

#ifdef ANDROID
    qInstallMessageHandler(myMessageHandler);
#endif

    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    app.setOrganizationName("Mobsya");
    app.setOrganizationDomain("mobsya.org");

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
