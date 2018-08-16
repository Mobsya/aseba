#include <QApplication>
#include <QFontDatabase>
#include <QDir>
#include <QDebug>
#include <QQmlApplicationEngine>
#include <QSurfaceFormat>
#ifdef QT_QML_DEBUG
#    include <QQmlDebuggingEnabler>
#endif
#include <aseba/qt-thymio-dm-client-lib/thymio-api.h>

int main(int argc, char** argv) {

#ifdef QT_QML_DEBUG
    QQmlDebuggingEnabler enabler;
#endif
    QApplication app(argc, argv);
    mobsya::register_qml_types();

    for(QString& file : QDir(":/fonts").entryList(QDir::Files)) {
        QString path = ":/fonts/" + file;
        qDebug() << path;
        QFontDatabase::addApplicationFont(path);
    }


    mobsya::ThymioDeviceManagerClient client;
    mobsya::ThymioDevicesModel model(client);

    QQmlApplicationEngine engine;

    QSurfaceFormat format;
    format.setSamples(16);
    QSurfaceFormat::setDefaultFormat(format);

    engine.rootContext()->setContextProperty("thymios", &model);

    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));
    return app.exec();
}
