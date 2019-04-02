#pragma once

#include <QObject>
#include <QUrl>

namespace mobsya {

class Launcher : public QObject {
    Q_OBJECT
public:
    Launcher(QObject* parent = nullptr);
    Q_INVOKABLE bool platformIsOsX() const;
#ifdef Q_OS_MAC
    Q_INVOKABLE bool launchOsXBundle(const QString& name, const QStringList & args) const;
    bool doLaunchOsXBundle(const QString& name, const QStringList & args) const;
#endif

    Q_INVOKABLE QString search_program(const QString& name) const;
    Q_INVOKABLE QUrl webapp_base_url(const QString& name) const;
    Q_INVOKABLE bool openUrl(const QUrl& url) const;
    Q_INVOKABLE bool launch_process(const QString& program, const QStringList& args) const;
    Q_INVOKABLE QByteArray readFileContent(QString path);
    Q_INVOKABLE QString getDownloadPath(const QUrl& url);

private:
    QStringList applicationsSearchPaths() const;
    QStringList webappsFolderSearchPaths() const;
};


}  // namespace mobsya
