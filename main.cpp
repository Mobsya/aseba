#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QTranslator>
#include "thymio-vpl2.h"

int main(int argc, char *argv[])
{
	QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
	QGuiApplication app(argc, argv);
	app.setOrganizationName("Thymio");
	app.setOrganizationDomain("thymio.org");
	app.setApplicationName("Thymio Flow");

	QTranslator qtTranslator;
	QLocale::system().name();
	qtTranslator.load("thymio-vpl2_" + QLocale::system().name(), ":/thymio-vpl2/translations/");
	app.installTranslator(&qtTranslator);

	thymioVPL2Init();

	QQmlApplicationEngine engine;
	engine.load(QUrl(QStringLiteral("qrc:/main.qml")));

	return app.exec();
}

