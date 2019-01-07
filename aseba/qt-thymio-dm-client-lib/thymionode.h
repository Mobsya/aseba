#pragma once

#include <QUuid>
#include <QString>
#include <QObject>
#include <memory>
#include <QUrl>
#include "request.h"

namespace mobsya {

struct ThymioVariable {
    Q_GADGET
    Q_PROPERTY(QVariant value READ value)
    Q_PROPERTY(bool isConstant READ isConstant)

public:
    ThymioVariable(QVariant value, bool constant = false) : m_value(value), m_constant(constant) {}

    QVariant value() const {
        return m_value;
    }

    bool isConstant() const {
        return m_constant;
    }

private:
    QVariant m_value;
    bool m_constant;
};


struct EventDescription {
    Q_GADGET
    Q_PROPERTY(QString name READ size)
    Q_PROPERTY(int size READ size)

public:
    EventDescription() : m_size(0) {}
    EventDescription(QString name, int size) : m_name(name), m_size(size) {}

    QString name() const {
        return m_name;
    }

    int size() const {
        return m_size;
    }

private:
    QString m_name;
    int m_size;
};


class ThymioDeviceManagerClientEndpoint;
class ThymioNode : public QObject {
    Q_OBJECT
public:
    enum class Status { Connected = 1, Available = 2, Busy = 3, Ready = 4, Disconnected = 5 };
    enum class NodeType { Thymio2 = 0, Thymio2Wireless = 1, SimulatedThymio2 = 2, DummyNode = 3, UnknownType = 4 };
    enum class NodeCapability { Rename = 0x01, ForceResetAndStop = 0x02 };

    using WatchableInfo = fb::WatchableInfo;
    using VMExecutionState = fb::VMExecutionState;
    using VMExecutionError = fb::VMExecutionError;


    using VariableMap = QMap<QString, ThymioVariable>;

    Q_ENUM(Status)
    Q_ENUM(VMExecutionState)
    Q_ENUM(NodeType)
    Q_ENUM(NodeCapability)
    Q_ENUM(WatchableInfo)
    Q_DECLARE_FLAGS(WatchFlags, WatchableInfo)
    Q_DECLARE_FLAGS(NodeCapabilities, NodeCapability)

    Q_PROPERTY(QUuid id READ uuid CONSTANT)
    Q_PROPERTY(QString name READ name WRITE rename NOTIFY nameChanged)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(VMExecutionState vmExecutionState READ vmExecutionState NOTIFY vmExecutionStateChanged)
    Q_PROPERTY(NodeCapabilities capabilities READ capabilities NOTIFY capabilitiesChanged)
    Q_PROPERTY(NodeType type READ type CONSTANT)


    ThymioNode(std::shared_ptr<ThymioDeviceManagerClientEndpoint>, const QUuid& uuid, const QString& name,
               mobsya::ThymioNode::NodeType type, QObject* parent = nullptr);

Q_SIGNALS:
    void modified();
    void nameChanged();
    void statusChanged();
    void capabilitiesChanged();
    void vmExecutionStateChanged();
    void vmExecutionStarted();
    void vmExecutionStopped();
    void vmExecutionPaused(int line = 0);
    void variablesChanged(const VariableMap& variables);
    void events(const VariableMap& variables);
    void eventsTableChanged(const QVector<EventDescription>& events);
    void vmExecutionError(VMExecutionError error, const QString& message, uint32_t line);

public:
    QUuid uuid() const;
    QString name() const;
    Status status() const;
    VMExecutionState vmExecutionState() const;
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
    Q_INVOKABLE Request lock();
    Q_INVOKABLE Request unlock();

    Q_INVOKABLE CompilationRequest compile_aseba_code(const QByteArray& code);
    Q_INVOKABLE CompilationRequest load_aseba_code(const QByteArray& code);

    Q_INVOKABLE Request stop();
    Q_INVOKABLE Request run();
    Q_INVOKABLE Request pause();
    Q_INVOKABLE Request reboot();
    Q_INVOKABLE Request step();
    Q_INVOKABLE Request stepToNextLine();
    Q_INVOKABLE Request writeProgramToDeviceMemory();

    Q_INVOKABLE BreakpointsRequest setBreakPoints(const QVector<unsigned>& breakpoints);

    Q_INVOKABLE Request setWatchVariablesEnabled(bool enabled);
    Q_INVOKABLE Request setWatchEventsEnabled(bool enabled);
    Q_INVOKABLE Request setWatchVMExecutionStateEnabled(bool enabled);

    Q_INVOKABLE AsebaVMDescriptionRequest fetchAsebaVMDescription();

    Q_INVOKABLE Request setVariabes(const VariableMap& variables);

    Q_INVOKABLE Request addEvent(const EventDescription& d);
    Q_INVOKABLE Request removeEvent(const QString& name);
    Q_INVOKABLE Request emitEvent(const QString& name, const QVariant& value);

private:
    void onExecutionStateChanged(const fb::VMExecutionStateChangedT& msg);
    void onVariablesChanged(VariableMap variables);
    void onEvents(VariableMap variables);
    void onEventsTableChanged(const QVector<EventDescription>& events);


    Request updateWatchedInfos();
    std::shared_ptr<ThymioDeviceManagerClientEndpoint> m_endpoint;
    QUuid m_uuid;
    QString m_name;
    Status m_status;
    VMExecutionState m_executionState;
    NodeCapabilities m_capabilities;
    NodeType m_type;
    WatchFlags m_watched_infos;
    QVector<EventDescription> m_events_table;

    friend ThymioDeviceManagerClientEndpoint;
};

}  // namespace mobsya

Q_DECLARE_METATYPE(mobsya::ThymioNode*)
Q_DECLARE_METATYPE(mobsya::ThymioNode::Status)
Q_DECLARE_METATYPE(mobsya::ThymioNode::NodeType)
Q_DECLARE_OPERATORS_FOR_FLAGS(mobsya::ThymioNode::NodeCapabilities)
