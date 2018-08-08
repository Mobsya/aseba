#include <QApplication>
#include <QFontDatabase>
#include <QDir>
#include <QDebug>
#include <QQmlApplicationEngine>
#include <aseba/qt-thymio-dm-client-lib/thymio-api.h>

int main(int argc, char** argv) {

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
    engine.rootContext()->setContextProperty("thymios", &model);

    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));
    return app.exec();
}
