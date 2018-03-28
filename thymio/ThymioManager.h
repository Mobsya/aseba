#pragma once
#include <QObject>
#include <QVector>
#include "ThymioProviderInfo.h"
#include "DeviceQtConnection.h"
#include <aseba/common/msg/msg.h>

namespace mobsya {

class ThymioNodeData {
public:
    ThymioNodeData(std::shared_ptr<DeviceQtConnection> connection, ThymioProviderInfo provider,
                   uint16_t node);

    void setVariable(QString name, const QList<int>& value);
    const ThymioProviderInfo& provider() const;
    uint16_t id() const;

private:
    friend class ThymioManager;
    void onMessageReceived(const std::shared_ptr<Aseba::Message>& message);
    void onDescriptionReceived(const Aseba::Description& description);
    void onVariableDescriptionReceived(const Aseba::NamedVariableDescription& description);
    void onFunctionDescriptionReceived(const Aseba::NativeFunctionDescription& description);
    void onEventDescriptionReceived(const Aseba::LocalEventDescription& description);
    void updateReadyness();

    std::shared_ptr<DeviceQtConnection> m_connection;
    ThymioProviderInfo m_provider;
    uint16_t m_nodeId;
    Aseba::TargetDescription m_description;
    Aseba::VariablesMap m_variablesMap;

    struct {
        u_int16_t variables{0}, event{0}, functions{0};
    } m_message_counter;
    bool m_ready;
};

class AbstractDeviceProber : public QObject {
    Q_OBJECT
public:
    AbstractDeviceProber(QObject* parent = nullptr);
    virtual std::vector<ThymioProviderInfo> getDevices() = 0;
Q_SIGNALS:
    void availabilityChanged();

public:
    virtual std::unique_ptr<QIODevice> openConnection(const ThymioProviderInfo& thymio) = 0;
};

class ThymioManager : public QObject {
    Q_OBJECT
public:
    ThymioManager(QObject* parent = nullptr);

public Q_SLOTS:
    void scanDevices();

private:
    std::shared_ptr<DeviceQtConnection> openConnection(const ThymioProviderInfo& info);
    void requestNodesList();

private Q_SLOTS:
    void onMessageReceived(const ThymioProviderInfo& provider, std::shared_ptr<Aseba::Message>);

private:
    ThymioProviderInfo first() const;
    bool hasDevice() const;

    std::map<ThymioProviderInfo, std::shared_ptr<DeviceQtConnection>> m_providers;
    std::vector<AbstractDeviceProber*> m_probes;
    std::vector<std::shared_ptr<ThymioNodeData>> m_thymios;
};

}    // namespace mobsya
