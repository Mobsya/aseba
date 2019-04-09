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
#include <QQuickWidget>
#include <QStyle>
#include <QScreen>
#include <QFileDialog>
#include <QPointer>
namespace mobsya {

Launcher::Launcher(ThymioDeviceManagerClient* client, QObject* parent) : m_client(client), QObject(parent) {}


bool Launcher::platformIsOsX() const {
#ifdef Q_OS_MACOS
    return true;
#else
    return false;

#endif
}

#ifdef Q_OS_MACOS
bool Launcher::launchOsXBundle(const QString& name, const QVariantMap &args) const {
    return doLaunchOsXBundle(name, args);
}
#endif

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
#ifdef Q_OS_OSX
    files.append(QFileInfo(QCoreApplication::applicationDirPath() + "/../Resources/").absolutePath());
#endif
    return files;
}

bool Launcher::launch_process(const QString& program, const QStringList& args) const {
    return QProcess::startDetached(program, args);
}

bool Launcher::launchPlayground() const {
#ifdef Q_OS_MAC
    return doLaunchPlaygroundBundle();
#else
    auto exe = search_program("asebaplayground");
    if(exe.isEmpty())
        return false;
    return launch_process(exe);
#endif
}

bool Launcher::openUrl(const QUrl& url) {
    qDebug() << url;

#ifdef MOBSYA_USE_WEBENGINE
    QUrl source("qrc:/qml/webview.qml");
#else
    QUrl source("qrc:/qml/webview_native.qml");
#endif


    auto e = new QQmlApplicationEngine(qobject_cast<QObject*>(this));
    disconnect(e, &QQmlApplicationEngine::quit, nullptr, nullptr);
    e->rootContext()->setContextProperty("Utils", qobject_cast<QObject*>(this));
    e->rootContext()->setContextProperty("appUrl", url);
    e->load(source);
    connect(e, &QQmlApplicationEngine::quit, &QQmlApplicationEngine::deleteLater);
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
