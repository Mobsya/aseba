#include "launcher.h"
#include <QStandardPaths>
#include <QGuiApplication>
#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QTemporaryFile>
#include <QDesktopServices>
#include <QTimer>
#include <QProcess>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QStyle>
#include <QScreen>
#include <QFileDialog>

namespace mobsya {

Launcher::Launcher(QObject* parent) : QObject(parent) {}


bool Launcher::platformIsOsX() const {
#ifdef Q_OS_MACOS
    return true;
#else
    return false;

#endif
}

bool Launcher::launchOsXBundle(const QString& name, const QStringList & args) const {
    return doLaunchOsXBundle(name, args);
}

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
    QStringList files{QFileInfo(QCoreApplication::applicationDirPath()).absolutePath()};
#ifdef Q_OS_LINUX
    files.append(QFileInfo(QCoreApplication::applicationDirPath() + "/../share/").absolutePath());
#endif
    return files;
}

bool Launcher::launch_process(const QString& program, const QStringList& args) const {
    return QProcess::startDetached(program, args);
}

bool Launcher::openUrl(const QUrl& url) const {
    QQmlApplicationEngine* engine = new QQmlApplicationEngine;
    engine->rootContext()->setContextProperty("Utils", (QObject*)this);
    engine->rootContext()->setContextProperty("appUrl", url);
    engine->load(QUrl("qrc:/qml/webview.qml"));

    QObject* topLevel = engine->rootObjects().value(0);
    QQuickWindow* window = qobject_cast<QQuickWindow*>(topLevel);

    window->setParent(0);
    window->setTransientParent(0);
    engine->setParent(window);
    window->setWidth(1024);
    window->setHeight(700);
    connect(window, SIGNAL(closing(QQuickCloseEvent*)), window, SLOT(deleteLater()));
    window->show();
    const auto screen_geom = window->screen()->availableGeometry();

    window->setGeometry((screen_geom.width() - window->width()) / 2, (screen_geom.height() - window->height()) / 2,
                        window->width(), window->height());

    return true;
}

QString Launcher::getDownloadPath(const QUrl& url) {
    QFileDialog d;
    const auto name = url.fileName();
    const auto extension = QFileInfo(name).suffix();
    d.setWindowTitle(tr("Save %1").arg(name));
    d.setNameFilter(QString("%1 (*.%1)").arg(extension));
    d.setAcceptMode(QFileDialog::AcceptSave);
    d.setFileMode(QFileDialog::AnyFile);
    d.setDefaultSuffix(extension);

    if(!d.exec())
        return {};
    const auto files = d.selectedFiles();
    if(files.size() != 1)
        return {};
    return files.first();
}

QUrl Launcher::webapp_base_url(const QString& name) const {
    static const std::map<QString, QString> default_folder_name = {{"blockly", "thymio_blockly"},
                                                                   {"scratch", "scratch"}};
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
