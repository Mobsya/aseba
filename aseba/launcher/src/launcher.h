#pragma once

#include <QObject>
#include <QUrl>
#include <qt-thymio-dm-client-lib/thymiodevicemanagerclient.h>

namespace mobsya {

class Launcher : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isZeroconfRunning READ isZeroconfRunning NOTIFY zeroconfStatusChanged)
public:
    Launcher(mobsya::ThymioDeviceManagerClient* client, QObject* parent = nullptr);
    Q_INVOKABLE bool platformIsOsX() const;
    Q_INVOKABLE bool platformIsLinux() const;
#ifdef Q_OS_MAC
    Q_INVOKABLE bool launchOsXBundle(const QString& name, const QVariantMap& args) const;
    bool doLaunchOsXBundle(const QString& name, const QVariantMap& args) const;
#endif

    Q_INVOKABLE QString search_program(const QString& name) const;
    Q_INVOKABLE QUrl webapp_base_url(const QString& name) const;
    Q_INVOKABLE bool openUrl(const QUrl& url);
    Q_INVOKABLE bool launch_process(const QString& program, const QStringList& args = {}) const;
    Q_INVOKABLE QByteArray readFileContent(QString path);
    Q_INVOKABLE QString getDownloadPath(const QUrl& url);
    Q_INVOKABLE bool launchPlayground() const;
#ifdef Q_OS_MAC
    bool doLaunchPlaygroundBundle() const;
#endif

    bool isZeroconfRunning() const;

Q_SIGNALS:
    void zeroconfStatusChanged() const;

private:
    mobsya::ThymioDeviceManagerClient* m_client;
    QStringList applicationsSearchPaths() const;
    QStringList webappsFolderSearchPaths() const;
};


}  // namespace mobsya
