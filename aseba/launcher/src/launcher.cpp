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
#include <QOperatingSystemVersion>

namespace mobsya {

Launcher::Launcher(ThymioDeviceManagerClient* client, QObject* parent) : QObject(parent), m_client(client) {
    connect(m_client, &ThymioDeviceManagerClient::zeroconfBrowserStatusChanged, this, &Launcher::zeroconfStatusChanged);

    setUseLocalBrowser(false);
    // On mac we use the native web view since chromium is not app-store compatible.
    // But on versions prior to High Sierra, the WebKit version shipped with
    // the OS cannot handle webassembly, which we require to run all of our web apps
    // So, instead, defer to the system browser - which is more likely to work
    // because the version of safari shipped with an up-to-date Sierra is more current
    // than the system's webkit
#ifdef Q_OS_OSX
    setUseLocalBrowser(true);
#endif
#ifdef Q_OS_ANDROID
    setUseLocalBrowser(true);
#endif
    readSettings();
}


bool Launcher::platformIsOsX() const {
#ifdef Q_OS_MACOS
    return true;
#else
    return false;
#endif
}
bool Launcher::platformIsIos() const {
#ifdef Q_OS_IOS
    return true;
#else
    return false;
#endif
}
bool Launcher::platformIsLinux() const {
#if (defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID))
    return true;
#else
    return false;
#endif
}
bool Launcher::platformIsAndroid() const {
#ifdef Q_OS_ANDROID
    return true;
#else
    return false;
#endif
}

Q_INVOKABLE bool Launcher::platformHasSerialPorts() const {
#if defined(Q_OS_IOS) || defined(Q_OS_ANDROID)
    return false;
#else
    return true;
#endif
}

#ifdef Q_OS_MACOS
bool Launcher::launchOsXBundle(const QString& name, const QVariantMap& args) const {
    return doLaunchOsXBundle(name, args);
}
#endif
#ifdef Q_OS_IOS
// Dummy implementation for iOS
bool Launcher::launchOsXBundle(const QString& name, const QVariantMap& args) const {
    return false;
}
#endif


bool Launcher::isPlaygroundAvailable() const {
// The playground is not supported on opengl ES platforms
#ifdef QT_OPENGL_ES
    return false;
#endif
// No Playground on mobile platforms
#if defined(Q_OS_IOS) || defined(Q_OS_ANDROID)
    return false;
#elif defined(Q_OS_OSX)
    // Search for a bundle on osx
    return hasOsXBundle("AsebaPlayground");
#else
    // Search for am executable (linux, windows)
    return !search_program("asebaplayground").isEmpty();
#endif
    return false;
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
#ifdef Q_OS_OSX
    files.append(QFileInfo(QCoreApplication::applicationDirPath() + "/../Resources/").absolutePath());
#endif
#ifdef Q_OS_IOS
    files.append(QFileInfo(QCoreApplication::applicationDirPath() + "/webapps/").absolutePath());
#endif
#ifdef Q_OS_ANDROID
    // See copyAndroidAssetsIntoAppDataIfRequired() in main.cpp.
    // files.append("assets:/");
    files.append(QStandardPaths::standardLocations(QStandardPaths::StandardLocation::AppLocalDataLocation).at(0));
#endif
    return files;
}

bool Launcher::launch_process(const QString& program, const QStringList& args) const {
#ifndef Q_OS_IOS
    return QProcess::startDetached(program, args);
#endif
}

bool Launcher::launchPlayground() const {
#ifdef Q_OS_OSX
    return doLaunchPlaygroundBundle();
#else
    auto exe = search_program("asebaplayground");
    if(exe.isEmpty())
        return false;
    return launch_process(exe);
#endif
}


static bool openUrlWithParameters(const QUrl& url) {
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

bool Launcher::openUrlWithExternal(const QUrl& url) const {
    QString urlPath = url.toString();
#ifdef Q_OS_ANDROID
    const QString LOCAL_FILES_PATH_SEPARATOR = "/files/";
#else
    const QString LOCAL_FILES_PATH_SEPARATOR = "/share/";
#endif
    int indexOfFiles = urlPath.indexOf(LOCAL_FILES_PATH_SEPARATOR);
    urlPath = urlPath.mid(indexOfFiles + LOCAL_FILES_PATH_SEPARATOR.length());

    // Use external web site
    QString urlAsString = QString("http://software.mobsya.org/%1").arg(urlPath);
    return QDesktopServices::openUrl(QUrl(urlAsString));
}

bool Launcher::openUrl(const QUrl& url) {

    qDebug() << url;

    if ( getUseLocalBrowser() ){
#ifdef Q_OS_LINUX
		return openUrlWithExternal(url);
#else
        return openUrlWithParameters(url);
#endif
    }
#ifdef Q_OS_IOS
    OpenUrlInNativeWebView(url);
    return true;
#endif

#ifdef MOBSYA_USE_WEBENGINE
    QUrl source("qrc:/qml/webview.qml");
#else
    QUrl source("qrc:/qml/webview_native.qml");
#endif
#ifdef Q_OS_ANDROID
    qDebug() << url.toString(QUrl::None);
#endif
    auto e = new QQmlApplicationEngine(qobject_cast<QObject*>(this));
    disconnect(e, &QQmlApplicationEngine::quit, nullptr, nullptr);
    e->rootContext()->setContextProperty("Utils", qobject_cast<QObject*>(this));
    e->rootContext()->setContextProperty("appUrl", url);
    e->load(source);

    // FIXME: On OSX, let the engine leak, otherwise the application
    // might crash
    // see https://bugreports.qt.io/browse/QTBUG-75165
#ifndef Q_OS_OSX
    connect(e, &QQmlApplicationEngine::quit, &QQmlApplicationEngine::deleteLater);
#endif
    return true;
}

/* **************
* it writes local application information to application settings
* for being available in next execution -
* the function must be called in the program exit and not all times
************** */
void Launcher::writeSettings(){
    QSettings settings("ThymioSuite", "Mobsya");
   // the use of local browser available
    settings.setValue("mainwindowuseLocalBrowser2",QVariant(getUseLocalBrowser()) );

}

/* **************
* it reads the application settings to set the application information
* the function must be called after the program load a
************** */
void Launcher::readSettings(){
    QSettings settings("ThymioSuite", "Mobsya");
    setUseLocalBrowser(settings.value("mainwindowuseLocalBrowser2",QVariant(useLocalBrowser)).toBool());
}


void Launcher::setUseLocalBrowser(bool checked){
    useLocalBrowser = checked;
}

bool Launcher::getUseLocalBrowser(){
    return useLocalBrowser;
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
                                                                   {"scratch", "scratch"},
                                                                   {"vpl3", "vpl3-thymio-suite"}};
    auto it = default_folder_name.find(name);
    if(it == default_folder_name.end())
        return {};
    for(auto dirpath : webappsFolderSearchPaths()) {
        QFileInfo d(dirpath + "/" + it->second);
        if(d.exists()) {
            auto q = QUrl::fromLocalFile(d.absoluteFilePath());
            qDebug() << QString("Web app %1 found in: %2 (%3)").arg(name, d.absoluteFilePath(), q.toString());
            return q;
        }
    }
    qDebug() << QString("Web app %1 not found!").arg(name);
    return QUrl();
}

QByteArray Launcher::readFileContent(QString path) {
    path.replace(QStringLiteral("qrc:/"), QStringLiteral(":/"));
    QFile f(path);
    f.open(QFile::ReadOnly);
    return f.readAll();
}

static bool exists(QString file) {
    return QFile::exists(file.replace("qrc:/", ":/"));
}

QString get_ui_language() {
    QLocale l;
    QStringList langs = QLocale().uiLanguages();
    if(langs.empty()) {
        return l.name();
    }
    return langs.first();
}

QString Launcher::uiLanguage() const {
    if (get_ui_language().mid(0, 2) == "zh")
        return get_ui_language().mid(0, 5).toLower();
    else
        return get_ui_language().mid(0, 2);
}

Q_INVOKABLE QString Launcher::filenameForLocale(QString pattern) {
    QString lang = get_ui_language();
    QString full = pattern.arg(lang);
    if(exists(full))
        return full;
    if(lang.mid(0,2) == "zh" )
        lang = pattern.arg(lang.mid(0, 5));
    else
        lang = pattern.arg(lang.mid(0, 2));
    if(exists(lang))
        return lang;
    QString en = pattern.arg("en");
    if(exists(en))
        return en;
    return pattern;
}

bool Launcher::isZeroconfRunning() const {
    triggerLocalNetworkPrivacyAlertObjC();
    return m_client->isZeroconfBrowserConnected();
}
#ifdef Q_OS_IOS
Q_INVOKABLE void Launcher::applicationStateChanged(Qt::ApplicationState state) {
    static Qt::ApplicationState lastState = Qt::ApplicationActive;

    if((lastState == Qt::ApplicationActive) && (state < Qt::ApplicationActive)) {
        // Check if it could be interessting to store the current connected device to force reconnect to it later, if
        // the browser is open
    } else if((lastState < Qt::ApplicationActive) && (state == Qt::ApplicationActive)) {
        // Fix the posisbility to reconnect from the thymio selection after the device sleeping, but does not provied
        // the browser to reconnect
        m_client->restartBrowser();
        zeroconfStatusChanged();
    }
    lastState = state;
}
#endif
}  // namespace mobsya
