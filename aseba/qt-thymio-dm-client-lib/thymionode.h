#pragma once

#include <QUuid>
#include <QString>
#include <QObject>
#include <memory>

namespace mobsya {
class ThymioDeviceManagerClientEndpoint;
class ThymioNode : public QObject {
    Q_OBJECT
public:
    enum class Status { connected = 1, available = 2, busy = 3, ready = 4, disconnected = 5 };
    enum class NodeType { Thymio2 = 0, Thymio2Wireless };

    Q_ENUM(Status)
    Q_ENUM(NodeType)

    Q_PROPERTY(QUuid uuid READ uuid CONSTANT)
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(NodeType type READ type CONSTANT)

    ThymioNode(std::shared_ptr<ThymioDeviceManagerClientEndpoint>, const QUuid& uuid, const QString& name,
               mobsya::ThymioNode::NodeType type);

Q_SIGNALS:
    void nameChanged();
    void statusChanged();

public:
    QUuid uuid() const;
    QString name() const;
    Status status() const;
    NodeType type() const;

    void setName(const QString& name);
    void setStatus(const Status& status);


private:
    std::shared_ptr<ThymioDeviceManagerClientEndpoint> m_endpoint;
    QUuid m_uuid;
    QString m_name;
    Status m_status;
    NodeType m_type;
};

}  // namespace mobsya

Q_DECLARE_METATYPE(mobsya::ThymioNode::Status)
