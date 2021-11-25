#include "MobsyaApplication.h"

bool mobsya::MobsyaApplication::event(QEvent* event) {
    if(event->type() == QEvent::FileOpen) {
        QFileOpenEvent* openEvent = static_cast<QFileOpenEvent*>(event);
        qDebug() << "Open file" << openEvent->file() << openEvent->url() << openEvent->url().path()
                 << openEvent->url().scheme();
        if(openEvent->url().scheme() == "mobsya" && openEvent->url().path() == "connect-to-device") {

            QUrlQuery q(openEvent->url());

            if(q.hasQueryItem("endpoint")) {
                QByteArray password = q.queryItemValue("password", QUrl::FullyDecoded).toUtf8();
                QString endpoint = q.queryItemValue("endpoint", QUrl::FullyDecoded);
                Q_EMIT endpointConnectionRequest(endpoint, password);
            }
            if(q.hasQueryItem("uuid")) {
                QUuid id = q.queryItemValue("uuid", QUrl::FullyDecoded);
                Q_EMIT deviceConnectionRequest(id);
            }
        }
    }
    return QApplication::event(event);
}
