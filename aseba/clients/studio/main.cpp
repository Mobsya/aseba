/*
    Aseba - an event-based framework for distributed robot control
    Created by Stéphane Magnenat <stephane at magnenat dot net> (http://stephane.magnenat.net)
    with contributions from the community.
    Copyright (C) 2007--2018 the authors, see authors.txt for details.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <signal.h>
#include <QTextCodec>
#include <QTranslator>
#include <QString>
#include <QLocale>
#include <QLibraryInfo>
#include <QDebug>
#include <QCommandLineParser>
#include <QTimer>
#include "MobsyaApplication.h"
#include "MainWindow.h"
#include <aseba/qt-thymio-dm-client-lib/thymiodevicemanagerclient.h>
#include <aseba/common/consts.h>


int main(int argc, char* argv[]) {
    qputenv("QT_LOGGING_TO_CONSOLE", QByteArray("0"));
    Q_INIT_RESOURCE(asebaqtabout);
    Q_INIT_RESOURCE(asebastudio);

	QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    mobsya::MobsyaApplication app(argc, argv);


    // Information used by QSettings with default constructor
    QCoreApplication::setOrganizationName(ASEBA_ORGANIZATION_NAME);
    QCoreApplication::setOrganizationDomain(ASEBA_ORGANIZATION_DOMAIN);
    QCoreApplication::setApplicationName(QStringLiteral("Aseba Studio"));

    auto load_trads = [](const QString& name, const QString& dir) {
        QTranslator* translator = new QTranslator(qApp);
        qDebug() << QLocale().name() << name << dir;
        if(translator->load(QLocale(), name, {}, dir)) {
            qApp->installTranslator(translator);
        } else {
            qDebug() << "Didn't load translation" << name;
        }
    };
    load_trads("asebastudio_", ":/translations/");
    load_trads("compiler_", ":/translations/");
    load_trads("qtabout_", ":/");
    load_trads("qtbase_", QLibraryInfo::location(QLibraryInfo::TranslationsPath));

    QCommandLineParser parser;
    QCommandLineOption uuid(QStringLiteral("uuid"),
                            QStringLiteral("Uuid of the target to connect to - can be specified multiple times"),
                            QStringLiteral("uuid"));
    QCommandLineOption ep(QStringLiteral("endpoint"), QStringLiteral("Endpoint to connect to"),
                          QStringLiteral("endpoint"));

    QCommandLineOption password(QStringLiteral("password"), QStringLiteral("Password associated with the endpoint"),
                                QStringLiteral("password"));
    parser.addOption(uuid);
    parser.addOption(ep);
    parser.addOption(password);
    parser.addHelpOption();
    parser.process(qApp->arguments());

    QVector<QUuid> targetUuids;
    for(auto&& id : parser.values(uuid)) {
        auto uuid = QUuid::fromString(id);
        if(!uuid.isNull())
            targetUuids.append(uuid);
    }

    mobsya::ThymioDeviceManagerClient thymioClient;

    QString s = parser.value(ep);
    if(!s.isEmpty()) {
        thymioClient.connectToRemoteUrlEndpoint(s, parser.value("password").toUtf8());
    }

    Aseba::MainWindow window(thymioClient, targetUuids);
    QObject::connect(&app, &mobsya::MobsyaApplication::deviceConnectionRequest, &window,
                     &Aseba::MainWindow::connectToDevice);
    QObject::connect(&app, &mobsya::MobsyaApplication::endpointConnectionRequest, &thymioClient,
                     &mobsya::ThymioDeviceManagerClient::connectToRemoteUrlEndpoint);
    window.show();
    return app.exec();
}
