#include "launcher.h"
#include <QStandardPaths>
#include <QCoreApplication>
#include <QDebug>
#include <QFileInfo>
#include <QDir>

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

}  // namespace mobsya