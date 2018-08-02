#pragma once
#include <QObject>
#include <qzeroconf.h>
#include <QMap>
#include <QUuid>
#include <memory>
#include <aseba/flatbuffers/fb_message_ptr.h>
#include "thymionode.h"

namespace mobsya {

class ThymioDeviceManagerClientEndpoint;

class ThymioDeviceManagerClient : public QObject {

public:
    ThymioDeviceManagerClient(QObject* parent = nullptr);

private Q_SLOTS:
    void onServiceAdded(QZeroConfService);
    void onServiceRemoved(QZeroConfService);
    void onMessage(const fb_message_ptr& msg);

private:
    struct SimpleNode {
        QUuid id;
        QString name;
        ThymioNode::Status status;
    };

    void onNodesChanged(const fb::NodesChanged&);
    void onNodesChanged(const std::vector<SimpleNode>& nodes);

    QVector<QZeroConfService> m_services;
    QZeroConf* m_register;
    QMap<QUuid, std::shared_ptr<ThymioDeviceManagerClientEndpoint>> m_endpoints;
    QMap<QUuid, std::shared_ptr<ThymioNode>> m_nodes;
};


}  // namespace mobsya
