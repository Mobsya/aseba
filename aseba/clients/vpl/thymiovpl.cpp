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
#include "MobsyaApplication.h"
#include <QTextCodec>
#include <QTranslator>
#include <QString>
#include <QLocale>
#include <QLibraryInfo>
#include <QDebug>
#include <QTimer>
#include <iostream>
#include <QCommandLineParser>
#include <QUuid>
#include <aseba/common/consts.h>
#include <qt-thymio-dm-client-lib/remoteconnectionrequest.h>
#include "ThymioVPLApplication.h"
#include "UsageLogger.h"

int main(int argc, char* argv[]) {
    Q_INIT_RESOURCE(asebaqtabout);
    Q_INIT_RESOURCE(asebastudio);

	QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    mobsya::MobsyaApplication app(argc, argv);

    // Information used by QSettings with default constructor
    QCoreApplication::setOrganizationName(ASEBA_ORGANIZATION_NAME);
    QCoreApplication::setOrganizationDomain(ASEBA_ORGANIZATION_DOMAIN);
    QCoreApplication::setApplicationName("Thymio VPL");

    const QString language(QLocale::system().name());

    QCommandLineParser parser;
    QCommandLineOption uuid(QStringLiteral("uuid"),
                            QStringLiteral("Uuid of the target to connect to - can be specified multiple times"),
                            QStringLiteral("uuid"));
    QCommandLineOption ep(QStringLiteral("endpoint"), QStringLiteral("Endpoint to connect to"),
                          QStringLiteral("endpoint"));
    parser.addOption(uuid);
    parser.addOption(ep);
    parser.addHelpOption();
    parser.process(qApp->arguments());

    const auto idStr = parser.value("uuid");
    const auto id = QUuid::fromString(idStr);

    auto load_trads = [](const QString& name, const QString& dir) {
        QTranslator* translator = new QTranslator(qApp);
        qDebug() << QLocale().name() << name << dir;
        if(translator->load(QLocale(), name, {}, dir)) {
            qApp->installTranslator(translator);
        } else {
            qDebug() << "Didn't load translation" << name;
        }
    };
    load_trads("asebastudio_", ":/translations");
    load_trads("compiler_", ":/translations");
    load_trads("qtabout_", ":/");
    load_trads("qtbase_", QLibraryInfo::location(QLibraryInfo::TranslationsPath));

    QVector<QUuid> targetUuids;
    for(auto&& id : parser.values(uuid)) {
        auto uuid = QUuid::fromString(id);
        if(!uuid.isNull())
            targetUuids.append(uuid);
    }

    mobsya::ThymioDeviceManagerClient thymioClient;


    QString s = parser.value(ep);
    if(!s.isEmpty()) {
        QUrl url(s);
        QString host = url.host();
        quint16 port = url.port();
        auto c = new mobsya::RemoteConnectionRequest(&thymioClient, host, port, qApp);
        QObject::connect(c, &mobsya::RemoteConnectionRequest::done, &mobsya::RemoteConnectionRequest::deleteLater);
        QObject::connect(c, &mobsya::RemoteConnectionRequest::error, &mobsya::RemoteConnectionRequest::deleteLater);
        QTimer::singleShot(0, c, &mobsya::RemoteConnectionRequest::start);
    }

    Aseba::ThymioVPLApplication vpl(&thymioClient, id);
    QObject::connect(&app, &mobsya::MobsyaApplication::deviceConnectionRequest, &vpl,
                     &Aseba::ThymioVPLApplication::connectToDevice);
    vpl.show();
    app.setOverrideCursor(Qt::ArrowCursor);
    return app.exec();
}
