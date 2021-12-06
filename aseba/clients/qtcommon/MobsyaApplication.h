#pragma once

#include <QObject>
#include <QUrl>
#include <QUrlQuery>
#include <QApplication>
#include <QFileOpenEvent>
#include <QDebug>
#include <QUuid>

namespace mobsya {
class MobsyaApplication : public QApplication {
    Q_OBJECT
public:
    MobsyaApplication(int& argc, char** argv) : QApplication(argc, argv) {}

protected:
    bool event(QEvent* event) override;
Q_SIGNALS:
    void deviceConnectionRequest(QUuid);
    void endpointConnectionRequest(QUrl, QByteArray);
};
}  // namespace mobsya
