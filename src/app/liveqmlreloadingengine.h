#pragma once
#include <QQuickView>

class QFileSystemWatcher;

class LiveQmlReloadingEngine : public QObject {
    Q_OBJECT
public:
    LiveQmlReloadingEngine();
    void load(const QString& path);

private Q_SLOTS:
    void reload();

private:
    QUrl m_rootFileUrl;
    QFileSystemWatcher* m_watcher;
    QQuickView m_window;
};
