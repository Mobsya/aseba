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
    Q_OBJECT
public:
    ThymioDeviceManagerClient(QObject* parent = nullptr);
    std::shared_ptr<ThymioNode> node(const QUuid& id) const;

private Q_SLOTS:
    void onServiceAdded(QZeroConfService);
    void onServiceRemoved(QZeroConfService);
    void onMessage(const fb_message_ptr& msg);

Q_SIGNALS:
    void nodeAdded(std::shared_ptr<ThymioNode>);
    void nodeRemoved(std::shared_ptr<ThymioNode>);
    void nodeModified(std::shared_ptr<ThymioNode>);

private:
    friend class ThymioDevicesModel;

    struct SimpleNode {
        QUuid id;
        QString name;
        ThymioNode::Status status;
        ThymioNode::NodeType type;
        ThymioNode::NodeCapabilities capabilities;
    };

    void onNodesChanged(const fb::NodesChanged&);
    void onNodesChanged(const std::vector<SimpleNode>& nodes);
    void onEndpointDisconnected();

    QVector<QZeroConfService> m_services;
    QZeroConf* m_register;
    QMap<QUuid, std::shared_ptr<ThymioDeviceManagerClientEndpoint>> m_endpoints;
    QMap<QUuid, std::shared_ptr<ThymioNode>> m_nodes;
};


}  // namespace mobsya
