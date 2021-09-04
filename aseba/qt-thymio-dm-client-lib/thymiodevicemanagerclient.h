#pragma once
#include <QObject>
#include <qzeroconf.h>
#include <QMap>
#include <QUuid>
#include <memory>
#include <aseba/flatbuffers/fb_message_ptr.h>
#include "thymionode.h"
#ifndef Q_MOC_RUN
#    include <range/v3/view/filter.hpp>
#    include <range/v3/view/transform.hpp>
#endif


namespace mobsya::detail {
template <class Key, class T>
class UnsignedQMap : public QMap<Key, T> {
public:
    using QMap<Key, T>::QMap;
    std::size_t size() const {
        return std::size_t(QMap<Key, T>::size());
    }
};
}  // namespace mobsya::detail

namespace mobsya {
class ThymioDeviceManagerClientEndpoint;

class ThymioDeviceManagerClient : public QObject {
    Q_OBJECT
    Q_PROPERTY(ThymioDeviceManagerClientEndpoint* localEndpoint READ qml_localEndpoint NOTIFY localEndpointChanged)


public:
    ThymioDeviceManagerClient(QObject* parent = nullptr);
    std::shared_ptr<ThymioNode> node(const QUuid& id) const;

    auto nodes(const QUuid& id) const {
        return ranges::view::all(m_nodes) |
            ranges::view::filter([&id](auto&& node) { return node->uuid() == id || node->group_id() == id; });
    }

    auto nodes() const {
        return ranges::view::all(m_nodes);
    }

    void requestDeviceManagersShutdown();
    void restartBrowser();
    bool isZeroconfBrowserConnected() const;
    void connectToRemoteUrlEndpoint(QUrl endpoint, QByteArray password);
    void connectToRemoteEndpoint(QString host, quint16 port, QByteArray password);

    bool hasEndpoint(QUuid) const;
    bool registerEndpoint(QUuid, std::shared_ptr<ThymioDeviceManagerClientEndpoint>);

private Q_SLOTS:
    void onServiceAdded(QZeroConfService);
    void onServiceRemoved(QZeroConfService);
    void onNodeAdded(std::shared_ptr<ThymioNode>);
    void onNodeRemoved(std::shared_ptr<ThymioNode>);
    void onLocalPeerChanged();

Q_SIGNALS:
    void nodeAdded(std::shared_ptr<ThymioNode>);
    void nodeRemoved(std::shared_ptr<ThymioNode>);
    void nodeModified(std::shared_ptr<ThymioNode>);
    void localEndpointChanged();
    void localPeerDisconnected();
    void zeroconfBrowserStatusChanged();


private:
    void doRegisterEndpoint(std::shared_ptr<ThymioDeviceManagerClientEndpoint>& endpoint);

    friend class ThymioDevicesModel;
    void onEndpointDisconnected();
    ThymioDeviceManagerClientEndpoint* qml_localEndpoint() const;

    QVector<QZeroConfService> m_services;
    QZeroConf* m_register;
    detail::UnsignedQMap<QUuid, std::shared_ptr<ThymioDeviceManagerClientEndpoint>> m_endpoints;
    detail::UnsignedQMap<QUuid, std::shared_ptr<ThymioNode>> m_nodes;
};


}  // namespace mobsya
