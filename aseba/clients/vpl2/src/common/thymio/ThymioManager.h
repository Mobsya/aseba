#pragma once
#include <aseba/common/msg/msg.h>
#include <QObject>
#include <QVector>
#include "ThymioProviderInfo.h"
#include "DeviceQtConnection.h"

namespace mobsya {

class ThymioNode : public QObject {
    Q_OBJECT
public:
    ThymioNode(std::shared_ptr<DeviceQtConnection> connection, ThymioProviderInfo provider, uint16_t node);

    void setVariable(QString name, const QList<int>& value);
    const ThymioProviderInfo& provider() const;

    Q_INVOKABLE uint16_t id() const;
    Q_INVOKABLE QString name() const;
    bool isReady() const;
    bool isConnected() const;

    auto description() {
        return &m_description;
    }

    void sendMessage(const Aseba::Message& message) const {
        m_connection->sendMessage(message);
    }

Q_SIGNALS:
    void ready();
    void userMessageReceived(int type, QList<int>);
    void connectedChanged();


private:
    friend class ThymioManager;
    friend class AsebaNode;
    void onMessageReceived(const std::shared_ptr<Aseba::Message>& message);
    void onDescriptionReceived(const Aseba::Description& description);
    void onVariableDescriptionReceived(const Aseba::NamedVariableDescription& description);
    void onFunctionDescriptionReceived(const Aseba::NativeFunctionDescription& description);
    void onEventDescriptionReceived(const Aseba::LocalEventDescription& description);
    void onUserMessageReceived(const Aseba::UserMessage& userMessage);
    void updateReadyness();

    std::shared_ptr<DeviceQtConnection> m_connection;
    ThymioProviderInfo m_provider;
    uint16_t m_nodeId;
    Aseba::TargetDescription m_description;
    Aseba::VariablesMap m_variablesMap;

    struct {
        uint16_t variables{0}, event{0}, functions{0};
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
    virtual std::shared_ptr<DeviceQtConnection> openConnection(const ThymioProviderInfo& thymio) = 0;
};

class ThymioManager : public QObject {
    Q_OBJECT
public:
    using Robot = std::shared_ptr<ThymioNode>;

    ThymioManager(QObject* parent = nullptr);
    std::size_t robotsCount() const;
    const Robot& at(std::size_t index) const;
    Robot robotFromId(unsigned id) const;

Q_SIGNALS:
    void robotAdded(const Robot& robot);
    void robotRemoved(const Robot& robot);

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
    std::vector<Robot> m_thymios;
};

}  // namespace mobsya
