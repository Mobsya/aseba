#include <QApplication>
#include <QFontDatabase>
#include <QDir>
#include <QDebug>
#include <QQmlApplicationEngine>

int main(int argc, char** argv) {

    QApplication app(argc, argv);
    for(QString& file : QDir(":/fonts").entryList(QDir::Files)) {
        QString path = ":/fonts/" + file;
        qDebug() << path;
        QFontDatabase::addApplicationFont(path);
    }
    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));
    return app.exec();
}
