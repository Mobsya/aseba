#include "launcher.h"
#include <QStandardPaths>
#include <QCoreApplication>
#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QTemporaryFile>
#include <QDesktopServices>
#include <QTimer>
#include <QProcess>

namespace mobsya {

Launcher::Launcher(QObject* parent) : QObject(parent) {}


QString Launcher::search_program(const QString& name) const {
    qDebug() << "Searching for " << name;
    auto path = QStandardPaths::findExecutable(name, applicationsSearchPaths());
    if(path.isEmpty()) {
        qDebug() << "Not found, search in standard locations";
        path = QStandardPaths::findExecutable(name);
    }
    if(path.isEmpty()) {
        qDebug() << name << " not found";
        return {};
    }
    qDebug() << name << "found: " << path;
    return path;
}

QStringList Launcher::applicationsSearchPaths() const {
    return {QCoreApplication::applicationDirPath()};
}

QStringList Launcher::webappsFolderSearchPaths() const {
    return {QFileInfo(QCoreApplication::applicationDirPath()).absolutePath()};
}

bool Launcher::launch_process(const QString& program, const QStringList& args) const {
    return QProcess::startDetached(program, args);
}

bool Launcher::openUrl(const QUrl& url) const {
    QTemporaryFile t(QDir::tempPath() + "/XXXXXX.html");
    t.setAutoRemove(false);
    if(!t.open())
        return false;

    t.write(QStringLiteral(R"(
<html><head>
  <meta http-equiv="refresh" content="0;URL='%1" />
</head></html>)")
                .arg(url.toString())
                .toUtf8());
    QTimer::singleShot(10000, [f = t.fileName()] { QFile::remove(f); });
    return QDesktopServices::openUrl(QUrl::fromLocalFile(t.fileName()));
}

QUrl Launcher::webapp_base_url(const QString& name) const {
    static const std::map<QString, QString> default_folder_name = {{"blockly", "thymio_blockly"}};
    auto it = default_folder_name.find(name);
    if(it == default_folder_name.end())
        return {};
    for(auto dirpath : webappsFolderSearchPaths()) {
        QFileInfo d(dirpath + "/" + it->second);
        if(d.exists()) {
            auto q = QUrl::fromLocalFile(d.absoluteFilePath());
            return q;
        }
    }
    return {};
}

QByteArray Launcher::readFileContent(QString path) {
    path.replace(QStringLiteral("qrc:/"), QStringLiteral(":/"));
    QFile f(path);
    f.open(QFile::ReadOnly);
    return f.readAll();
}

}  // namespace mobsya