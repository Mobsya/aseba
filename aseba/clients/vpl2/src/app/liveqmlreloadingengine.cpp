#include "liveqmlreloadingengine.h"
#include <QFileInfo>
#include <QDir>
#include <QFileSystemWatcher>
#include <QtDebug>
#include <QQmlEngine>

static void scan_files(const QString& root, QStringList& files) {
    for(auto&& entry : QDir(root).entryInfoList(QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs)) {
        if(entry.isDir()) {
            scan_files(entry.absoluteFilePath(), files);
            continue;
        }
        if(entry.isFile()) {
            files.append(entry.absoluteFilePath());
        }
    }
}


LiveQmlReloadingEngine::LiveQmlReloadingEngine() : m_watcher(new QFileSystemWatcher(this)) {
    connect(m_watcher, &QFileSystemWatcher::fileChanged, this, &LiveQmlReloadingEngine::reload);
    connect(m_watcher, &QFileSystemWatcher::fileChanged, [this](const QString& path) {
        m_watcher->addPath(path);  // rearm the signal
    });
}

void LiveQmlReloadingEngine::load(const QString& path) {
    QString dir = QFileInfo(path).absolutePath();
    m_rootFileUrl = QUrl::fromLocalFile(path);
    QStringList files;
    scan_files(dir, files);
    m_watcher->removePaths(m_watcher->files());
    m_watcher->addPaths(files);
    reload();
}

void LiveQmlReloadingEngine::reload() {
    qDebug() << "reloading";
    m_window.engine()->clearComponentCache();
    m_window.setResizeMode(QQuickView::SizeRootObjectToView);
    m_window.setMinimumSize({800, 600});
    m_window.setSource(m_rootFileUrl);
    m_window.show();
    //   m_engine.load(QUrl());
    //   m_engine.clearComponentCache();
    //  m_engine.load(m_rootFileUrl);
}
