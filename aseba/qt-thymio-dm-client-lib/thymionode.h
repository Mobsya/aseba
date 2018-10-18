#pragma once

#include <QUuid>
#include <QString>
#include <QObject>
#include <memory>
#include <QUrl>
#include "request.h"

namespace mobsya {
class ThymioDeviceManagerClientEndpoint;
class ThymioNode : public QObject {
    Q_OBJECT
public:
    enum class Status { Connected = 1, Available = 2, Busy = 3, Ready = 4, Disconnected = 5 };
    enum class NodeType { Thymio2 = 0, Thymio2Wireless = 1, SimulatedThymio2 = 2, DummyNode = 3, UnknownType = 4 };
    enum class NodeCapability { Rename = 0x01, ForceResetAndStop = 0x02 };

    Q_ENUM(Status)
    Q_ENUM(NodeType)
    Q_ENUM(NodeCapability)
    Q_DECLARE_FLAGS(NodeCapabilities, NodeCapability)

    Q_PROPERTY(QUuid id READ uuid CONSTANT)
    Q_PROPERTY(QString name READ name WRITE rename NOTIFY nameChanged)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(NodeCapabilities capabilities READ capabilities NOTIFY capabilitiesChanged)
    Q_PROPERTY(NodeType type READ type CONSTANT)


    ThymioNode(std::shared_ptr<ThymioDeviceManagerClientEndpoint>, const QUuid& uuid, const QString& name,
               mobsya::ThymioNode::NodeType type, QObject* parent = nullptr);

Q_SIGNALS:
    void modified();
    void nameChanged();
    void statusChanged();
    void capabilitiesChanged();

public:
    QUuid uuid() const;
    QString name() const;
    Status status() const;
    NodeType type() const;
    NodeCapabilities capabilities();
    Q_INVOKABLE QUrl websocketEndpoint() const;

    void setName(const QString& name);
    void setStatus(const Status& status);
    void setCapabilities(const NodeCapabilities& capabilities);

    std::shared_ptr<ThymioDeviceManagerClientEndpoint> endpoint() const {
        return m_endpoint;
    }

    Q_INVOKABLE Request rename(const QString& newName);
    Q_INVOKABLE Request stop();
    Q_INVOKABLE Request lock();
    Q_INVOKABLE Request unlock();
    Q_INVOKABLE CompilationRequest compile_aseba_code(const QByteArray& code);
    Q_INVOKABLE CompilationRequest load_aseba_code(const QByteArray& code);

private:
    std::shared_ptr<ThymioDeviceManagerClientEndpoint> m_endpoint;
    QUuid m_uuid;
    QString m_name;
    Status m_status;
    NodeCapabilities m_capabilities;
    NodeType m_type;
};

}  // namespace mobsya

Q_DECLARE_METATYPE(mobsya::ThymioNode*)
Q_DECLARE_METATYPE(mobsya::ThymioNode::Status)
Q_DECLARE_METATYPE(mobsya::ThymioNode::NodeType)
Q_DECLARE_OPERATORS_FOR_FLAGS(mobsya::ThymioNode::NodeCapabilities)
