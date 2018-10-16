#pragma once

#include <QObject>
#include <QUrl>

namespace mobsya {

class Launcher : public QObject {
    Q_OBJECT
public:
    Launcher(QObject* parent = nullptr);
    Q_INVOKABLE QString search_program(const QString& name) const;
    Q_INVOKABLE QUrl webapp_base_url(const QString& name) const;
    Q_INVOKABLE bool openUrl(const QUrl& url) const;
    Q_INVOKABLE bool launch_process(const QString& program, const QStringList& args) const;

private:
    QStringList applicationsSearchPaths() const;
    QStringList webappsFolderSearchPaths() const;
};


}  // namespace mobsya