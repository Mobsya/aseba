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
    // Q_PROPERTY(bool isConstant READ isConstant)

public:
    ThymioVariable(QVariant value) : m_value(value) {}

    QVariant value() const {
        return m_value;
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
class ThymioGroup;
class ThymioNode : public QObject {
    Q_OBJECT
public:
    enum class Status { Connected = 1, Available = 2, Busy = 3, Ready = 4, Disconnected = 5 };
    enum class NodeType { Thymio2 = 0, Thymio2Wireless = 1, SimulatedThymio2 = 2, DummyNode = 3, UnknownType = 4 };
    enum class NodeCapability { Rename = 0x01, ForceResetAndStop = 0x02, FirmwareUpgrade = 0x04 };

    using WatchableInfo = fb::WatchableInfo;
    using VMExecutionState = fb::VMExecutionState;
    using VMExecutionError = fb::VMExecutionError;


    using VariableMap = QMap<QString, ThymioVariable>;
    using EventMap = QMap<QString, QVariant>;

    Q_ENUM(Status)
    Q_ENUM(VMExecutionState)
    Q_ENUM(NodeType)
    Q_ENUM(NodeCapability)
    Q_ENUM(WatchableInfo)
    Q_DECLARE_FLAGS(WatchFlags, WatchableInfo)
    Q_DECLARE_FLAGS(NodeCapabilities, NodeCapability)

    Q_PROPERTY(QUuid id READ uuid CONSTANT)
    Q_PROPERTY(QUuid group_id READ group_id NOTIFY groupChanged)
    Q_PROPERTY(QString name READ name WRITE rename NOTIFY nameChanged)
    Q_PROPERTY(Status status READ status NOTIFY statusChanged)
    Q_PROPERTY(VMExecutionState vmExecutionState READ vmExecutionState NOTIFY vmExecutionStateChanged)
    Q_PROPERTY(NodeCapabilities capabilities READ capabilities NOTIFY capabilitiesChanged)
    Q_PROPERTY(NodeType type READ type CONSTANT)
    Q_PROPERTY(bool isInGroup READ isInGroup NOTIFY groupChanged)

    Q_PROPERTY(QString fwVersion READ fwVersion NOTIFY modified)
    Q_PROPERTY(QString fwVersionAvailable READ fwVersionAvailable NOTIFY modified)
    Q_PROPERTY(bool hasAvailableFirmwareUpdate READ hasAvailableFirmwareUpdate NOTIFY modified)


    Q_PROPERTY(QObject* endpoint READ endpoint_qml CONSTANT)


    ThymioNode(std::shared_ptr<ThymioDeviceManagerClientEndpoint>, const QUuid& uuid, const QString& name,
               mobsya::ThymioNode::NodeType type, QObject* parent = nullptr);
Q_SIGNALS:
    void modified();
    void groupChanged();
    void nameChanged();
    void statusChanged();
    void capabilitiesChanged();
    void vmExecutionStateChanged();
    void vmExecutionStarted();
    void vmExecutionStopped();
    void vmExecutionPaused(int line = 0);
    void variablesChanged(const VariableMap& variables, const QDateTime& timestamp);
    void groupVariablesChanged(const VariableMap& variables, const QDateTime& timestamp);
    void events(const EventMap& variables, const QDateTime& timestamp);
    void eventsTableChanged(const QVector<EventDescription>& events);
    void vmExecutionError(VMExecutionError error, const QString& message, uint32_t line);
    void ReadyBytecode(const QString& text);
    void scratchpadChanged(const QString& test, fb::ProgrammingLanguage language);
    void firmwareUpdateProgress(double);

public:
    QUuid uuid() const;
    QUuid group_id() const;
    std::shared_ptr<ThymioGroup> group() const;
    QString name() const;
    Status status() const;
    VMExecutionState vmExecutionState() const;
    NodeType type() const;
    NodeCapabilities capabilities() const;
    bool isInGroup() const;

    Q_INVOKABLE QUrl websocketEndpoint() const;

    void setGroup(std::shared_ptr<ThymioGroup> group);
    void setName(const QString& name);
    void setStatus(const Status& status);
    void setCapabilities(const NodeCapabilities& capabilities);

    std::shared_ptr<ThymioDeviceManagerClientEndpoint> endpoint() const {
        return m_endpoint;
    }

    QString fwVersionAvailable() const;
    QString fwVersion() const;
    bool hasAvailableFirmwareUpdate() const;

    Q_INVOKABLE Request rename(const QString& newName);
    Q_INVOKABLE Request lock();
    Q_INVOKABLE Request unlock();

    Q_INVOKABLE CompilationRequest compile_aseba_code(const QByteArray& code);
    Q_INVOKABLE CompilationRequest load_aseba_code(const QByteArray& code);
    Q_INVOKABLE CompilationRequest save_aseba_code(const QByteArray& code);

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
    Q_INVOKABLE Request setWatchSharedVariablesEnabled(bool enabled);
    Q_INVOKABLE Request setWatchEventsDescriptionEnabled(bool enabled);
    Q_INVOKABLE Request setWatchVMExecutionStateEnabled(bool enabled);

    Q_INVOKABLE AsebaVMDescriptionRequest fetchAsebaVMDescription();

    Q_INVOKABLE Request setVariables(const VariableMap& variables);
    Q_INVOKABLE Request setGroupVariables(const VariableMap& variables);

    Q_INVOKABLE Request addEvent(const EventDescription& d);
    Q_INVOKABLE Request removeEvent(const QString& name);
    Q_INVOKABLE Request emitEvent(const QString& name, const QVariant& value);

    Q_INVOKABLE Request setScratchPad(const QByteArray& data);

    Q_INVOKABLE Request upgradeFirmware();

private:
    Q_INVOKABLE QObject* endpoint_qml() const;

    void onExecutionStateChanged(const fb::VMExecutionStateChangedT& msg);
    void onVariablesChanged(VariableMap variables, const QDateTime& timestamp);

    void onEvents(const EventMap& variables, const QDateTime& timestamp);

    void onGroupVariablesChanged(VariableMap variables, const QDateTime& timestamp);
    void onEventsTableChanged(const QVector<EventDescription>& events);
    void onScratchpadChanged(const QString& text, fb::ProgrammingLanguage language);
    void onReadyBytecode(const QString& text);
    void onSendBytecode(const QString& text, fb::ProgrammingLanguage language);    
    void onFirmwareUpgradeProgress(double);

    Request updateWatchedInfos();
    std::shared_ptr<ThymioDeviceManagerClientEndpoint> m_endpoint;
    std::shared_ptr<ThymioGroup> m_group;
    QUuid m_uuid;
    QString m_name;
    Status m_status;
    QString m_fw_version, m_fw_version_available;
    VMExecutionState m_executionState;
    NodeCapabilities m_capabilities;
    NodeType m_type;
    WatchFlags m_watched_infos;

    friend class ThymioDeviceManagerClientEndpoint;
    friend class ThymioGroup;
};

struct Scratchpad {
    QUuid id;
    QUuid nodeId;
    QString name;
    QString code;
    fb::ProgrammingLanguage language;
    bool deleted;
};

class ThymioGroup : public QObject {
    Q_OBJECT

public:
    using VariableMap = ThymioNode::VariableMap;
    ThymioGroup(std::shared_ptr<ThymioDeviceManagerClientEndpoint>, const QUuid& id);

    QUuid uuid() const;

    Q_INVOKABLE Request setSharedVariables(const VariableMap& variables);
    Q_INVOKABLE Request addEvent(const EventDescription& d);
    Q_INVOKABLE Request removeEvent(const QString& name);
    Q_INVOKABLE Request loadAesl(const QByteArray& code);

    Q_INVOKABLE Request clearEventsAndVariables();

    QVector<EventDescription> eventsDescriptions() const;
    VariableMap sharedVariables() const;
    std::vector<std::shared_ptr<ThymioNode>> nodes() const;
    QVector<Scratchpad> scratchpads() const;
    void watchScratchpadsChanges(bool);

Q_SIGNALS:
    void scratchPadChanged(const Scratchpad& scratchpad);
    void groupChanged();

private:
    void addNode(std::shared_ptr<ThymioNode>);
    void removeNode(std::shared_ptr<ThymioNode>);

    void onSharedVariablesChanged(VariableMap variables, const QDateTime& timestamp);
    void onEventsDescriptionsChanged(const QVector<EventDescription>& events);
    void onScratchpadChanged(const Scratchpad& scratchpad);


    QUuid m_group_id;
    std::shared_ptr<ThymioDeviceManagerClientEndpoint> m_endpoint;

    VariableMap m_shared_variables;
    QVector<EventDescription> m_events_table;
    std::vector<std::weak_ptr<ThymioNode>> m_nodes;
    QVector<Scratchpad> m_scratchpads;
    ThymioNode::WatchFlags m_watched_infos = 0;

    friend class ThymioNode;
    friend class ThymioDeviceManagerClientEndpoint;
};


}  // namespace mobsya

Q_DECLARE_METATYPE(mobsya::ThymioNode*)
Q_DECLARE_METATYPE(mobsya::ThymioNode::Status)
Q_DECLARE_METATYPE(mobsya::ThymioNode::NodeType)
Q_DECLARE_OPERATORS_FOR_FLAGS(mobsya::ThymioNode::NodeCapabilities)
