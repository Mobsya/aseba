#pragma once
#include <QObject>
#include <QTcpSocket>
#include <aseba/flatbuffers/fb_message_ptr.h>
#include <QUrl>
#include <QDateTime>
#include <QTimer>
#include <QQmlListProperty>
#include "request.h"
#include "thymionode.h"
#include "thymio2wirelessdongle.h"

namespace mobsya {
class ThymioDeviceManagerClientEndpoint : public QObject,
                                          public std::enable_shared_from_this<ThymioDeviceManagerClientEndpoint> {
    Q_OBJECT
    Q_PROPERTY(QString hostName READ hostName CONSTANT)
    Q_PROPERTY(bool isLocalhostPeer READ isLocalhostPeer CONSTANT)
    Q_PROPERTY(QUrl websocketConnectionUrl READ websocketConnectionUrl CONSTANT)
    Q_PROPERTY(Thymio2WirelessDonglesManager* donglesManager READ donglesManager CONSTANT)
    Q_PROPERTY(QQmlListProperty<ThymioNode> nodes READ qml_nodes NOTIFY nodesChanged)


public:
    ThymioDeviceManagerClientEndpoint(QTcpSocket* socket, QString host, QObject* parent = nullptr);
    ~ThymioDeviceManagerClientEndpoint();

public:
    std::shared_ptr<ThymioNode> node(const QUuid& id) const;

    const QTcpSocket* socket() const {
        return m_socket;
    }

    QHostAddress peerAddress() const;
    QString hostName() const;

    QUrl websocketConnectionUrl() const;
    QUrl tcpConnectionUrl() const;
    void setWebSocketMatchingPort(quint16 port);

    bool isLocalhostPeer() const;
    Request requestDeviceManagerShutdown();


    Request renameNode(const ThymioNode& node, const QString& newName);
    Request setNodeExecutionState(const ThymioNode& node, fb::VMExecutionStateCommand cmd);
    BreakpointsRequest setNodeBreakPoints(const ThymioNode& node, const QVector<unsigned>& breakpoints);
    Request lock(const ThymioNode& node);
    Request unlock(const ThymioNode& node);
    CompilationRequest send_code(const ThymioNode& node, const QByteArray& code, fb::ProgrammingLanguage language,
                                 fb::CompilationOptions opts);
    Request send_aesl(const ThymioGroup& group, const QByteArray& code);
    Request set_watch_flags(const QUuid& node, int flags);
    AsebaVMDescriptionRequest fetchAsebaVMDescription(const ThymioNode& node);
    Request setNodeVariables(const ThymioNode& node, const ThymioNode::VariableMap& vars);
    Request setGroupVariables(const ThymioGroup& group, const ThymioNode::VariableMap& vars);
    Request setNodeEventsTable(const QUuid& id, const QVector<EventDescription>& events);
    Request emitNodeEvents(const ThymioNode& node, const ThymioNode::EventMap& vars);
    Request setScratchPad(const QUuid& id, const QByteArray& data, fb::ProgrammingLanguage language);
    Request upgradeFirmware(const QUuid& id);

    Q_INVOKABLE Request enableWirelessPairingMode();
    Q_INVOKABLE Request disableWirelessPairingMode();
    Thymio2WirelessDonglePairingRequest pairThymio2Wireless(const QUuid& dongleId, const QUuid& nodeId,
                                                            quint16 networkId, quint8 channel);


    Thymio2WirelessDonglesManager* donglesManager() const;

private Q_SLOTS:
    void onReadyRead();
    void onConnected();
    void cancelAllRequests();
    void checkSocketHealth();
    void write(const flatbuffers::DetachedBuffer& buffer);
    void write(const tagged_detached_flatbuffer& buffer);

Q_SIGNALS:
    void nodesChanged();
    void nodeAdded(std::shared_ptr<ThymioNode>);
    void nodeRemoved(std::shared_ptr<ThymioNode>);
    void nodeModified(std::shared_ptr<ThymioNode>);

    void onMessage(const fb_message_ptr& msg) const;
    void disconnected();
    void localPeerChanged();
    void handshakeCompleted(QUuid id);

private:
    template <typename T>
    T prepare_request() {
        T r = T::make_request();
        m_pending_requests.insert(r.id(), r.get_ptr());
        return r;
    }

    QQmlListProperty<ThymioNode> qml_nodes();

    void onNodesChanged(const fb::NodesChangedT& nc_msg);
    void handleIncommingMessage(const fb_message_ptr& msg);
    detail::RequestDataBase::shared_ptr get_request(detail::RequestDataBase::request_id);
    std::vector<std::shared_ptr<ThymioNode>> nodes(const QUuid& node_or_group_id) const;
    std::shared_ptr<ThymioGroup> group_from_group_id(const QUuid& id);
    std::shared_ptr<ThymioGroup> group_from_id(const QUuid& id);

    Request setVariables(const QUuid& id, const ThymioNode::VariableMap& vars);


    static flatbuffers::Offset<fb::NodeId> serialize_uuid(flatbuffers::FlatBufferBuilder& fb, const QUuid& uuid);
    QMap<QUuid, std::shared_ptr<ThymioNode>> m_nodes;
    QTcpSocket* m_socket;
    quint32 m_message_size;
    QUuid m_serverUUid;
    quint16 m_ws_port = 0;
    bool m_islocalhostPeer = false;
    QMap<detail::RequestDataBase::request_id, detail::RequestDataBase::shared_ptr> m_pending_requests;
    QString m_host_name;
    QDateTime m_last_message_reception_date;
    QTimer* m_socket_health_check_timer;
    Thymio2WirelessDonglesManager* m_dongles_manager;
};

}  // namespace mobsya


Q_DECLARE_METATYPE(mobsya::ThymioDeviceManagerClientEndpoint*)
Q_DECLARE_METATYPE(QQmlListProperty<mobsya::ThymioNode>)
