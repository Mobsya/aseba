#pragma once

#include <QObject>
#include <QUrl>
#include <qt-thymio-dm-client-lib/thymiodevicemanagerclient.h>
#include <qt-thymio-dm-client-lib/remoteconnectionrequest.h>
#include <QDir>
#include <QCoreApplication>
#include <QSettings>

namespace mobsya {

class Launcher : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isZeroconfRunning READ isZeroconfRunning NOTIFY zeroconfStatusChanged)
    Q_PROPERTY(bool isPlaygroundAvailable READ isPlaygroundAvailable CONSTANT)
    Q_PROPERTY(QString uiLanguage READ uiLanguage)
public:
    Launcher(mobsya::ThymioDeviceManagerClient* client, QObject* parent = nullptr);

    QString uiLanguage() const;
    bool isPlaygroundAvailable() const;
    Q_INVOKABLE bool platformIsOsX() const;
    Q_INVOKABLE bool platformIsIos() const;
    Q_INVOKABLE bool platformIsAndroid() const;
    Q_INVOKABLE bool platformIsLinux() const;
    Q_INVOKABLE bool platformHasSerialPorts() const;
    Q_INVOKABLE void setUseLocalBrowser(bool allow);
    Q_INVOKABLE bool getUseLocalBrowser() const;
    Q_INVOKABLE void setAllowRemoteConnections(bool allow);
    Q_INVOKABLE bool getAllowRemoteConnections() const;
    Q_INVOKABLE bool isApplicationInstalled(const QString& name) const;

#ifdef Q_OS_OSX
    Q_INVOKABLE bool launchOsXBundle(const QString& name, const QVariantMap& args) const;
    bool doLaunchOsXBundle(const QString& name, const QVariantMap& args) const;
    bool hasOsXBundle(const QString& name) const;
#endif
#ifdef Q_OS_IOS
    void OpenUrlInNativeWebView(const QUrl& url);
    // dummy implementation
    Q_INVOKABLE bool launchOsXBundle(const QString& name, const QVariantMap& args) const;
#endif

    Q_INVOKABLE QString search_program(const QString& name) const;
    Q_INVOKABLE QUrl webapp_base_url(const QString& name) const;
    bool openUrlWithExternal(const QUrl& url) const;
    Q_INVOKABLE bool openUrl(const QUrl& url);
    Q_INVOKABLE bool launch_process(const QString& program, const QStringList& args = {}) const;
    Q_INVOKABLE QByteArray readFileContent(QString path);
    Q_INVOKABLE QString filenameForLocale(QString pattern);
    Q_INVOKABLE QString getDownloadPath(const QUrl& url);
    Q_INVOKABLE bool launchPlayground() const;

    Q_INVOKABLE RemoteConnectionRequest* connectToServer(const QString& host, quint16 port, QByteArray password) const;


#ifdef Q_OS_OSX
    bool doLaunchPlaygroundBundle() const;
    bool isThonnyInstalled() const;
	bool doLaunchThonnyBundle() const;
#endif
#if defined(Q_OS_IOS) || defined(Q_OS_ANDROID)
    Q_INVOKABLE void applicationStateChanged(Qt::ApplicationState state);
#endif

    bool isZeroconfRunning() const;

    ThymioDeviceManagerClient* client() const;

    /* *************
     * Launcher Application Settings -
     ************** */
    void writeSettings();
    void readSettings();

Q_SIGNALS:
    void zeroconfStatusChanged() const;
    void remoteConnectionsAllowedChanged() const;

private:
    mobsya::ThymioDeviceManagerClient* m_client;
    QStringList applicationsSearchPaths() const;
    QStringList webappsFolderSearchPaths() const;
    bool useLocalBrowser;
    bool allowRemoteConnections;
};


}  // namespace mobsya
