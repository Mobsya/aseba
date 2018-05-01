#include "ThymioManager.h"
#include <QDebug>
#include <vector>
#include <QTimer>

#ifdef Q_OS_ANDROID
#    include "AndroidSerialDeviceProber.h"
#endif
#if(defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)) || defined(Q_OS_MAC) || defined(Q_OS_WIN)
#    include "UsbSerialDeviceProber.h"
#endif

#include "NetworkDeviceProber.h"

#include "aseba/compiler/compiler.h"

namespace mobsya {

AbstractDeviceProber::AbstractDeviceProber(QObject* parent) : QObject(parent) {}


ThymioNode::ThymioNode(std::shared_ptr<DeviceQtConnection> connection, ThymioProviderInfo provider, uint16_t node)
    : m_connection(connection), m_provider(provider), m_nodeId(node), m_ready(false) {

    connect(connection.get(), &DeviceQtConnection::connectionStatusChanged, this, &ThymioNode::connectedChanged);
    connect(this, &ThymioNode::connectedChanged, this, &ThymioNode::ready);
}

void ThymioNode::setVariable(QString name, const QList<int>& value) {
    uint16_t start = uint16_t(m_variablesMap[name.toStdWString()].first);
    Aseba::VariablesDataVector variablesVector(value.begin(), value.end());
    Aseba::SetVariables message(m_nodeId, start, variablesVector);
    m_connection->sendMessage(message);
}

void ThymioNode::onMessageReceived(const std::shared_ptr<Aseba::Message>& message) {
    switch(message->type) {
        case ASEBA_MESSAGE_NODE_PRESENT: break;
        case ASEBA_MESSAGE_DESCRIPTION:
            onDescriptionReceived(*static_cast<const Aseba::Description*>(message.get()));
            break;
        case ASEBA_MESSAGE_NAMED_VARIABLE_DESCRIPTION:
            onVariableDescriptionReceived(*static_cast<const Aseba::NamedVariableDescription*>(message.get()));
            break;
        case ASEBA_MESSAGE_LOCAL_EVENT_DESCRIPTION:
            onEventDescriptionReceived(*static_cast<const Aseba::LocalEventDescription*>(message.get()));
            break;
        case ASEBA_MESSAGE_NATIVE_FUNCTION_DESCRIPTION:
            onFunctionDescriptionReceived(*static_cast<const Aseba::NativeFunctionDescription*>(message.get()));
            break;
        default:
            if(auto um = dynamic_cast<Aseba::UserMessage*>(message.get())) {  // This is so terrible...
                onUserMessageReceived(*um);
            }
    }
}

void ThymioNode::onDescriptionReceived(Aseba::Description description) {
    m_description = std::move(description);
    updateReadyness();
}


void ThymioNode::onVariableDescriptionReceived(Aseba::NamedVariableDescription description) {
    auto it = std::find_if(std::begin(m_description.namedVariables), std::end(m_description.namedVariables),
                           [&description](auto&& variable) { return variable.name == description.name; });
    if(it != std::end(m_description.namedVariables))
        return;

    qDebug() << bool(it != std::end(m_description.namedVariables)) << m_description.namedVariables.size()
             << m_message_counter.variables;

    if(m_description.namedVariables.size() == m_message_counter.variables) {
        m_description.namedVariables.resize(m_description.namedVariables.size() + 1);
    }

    m_description.namedVariables[m_message_counter.variables++] = std::move(description);
    updateReadyness();
}
void ThymioNode::onFunctionDescriptionReceived(Aseba::NativeFunctionDescription description) {
    auto it = std::find_if(std::begin(m_description.nativeFunctions), std::end(m_description.nativeFunctions),
                           [&description](auto&& function) { return function.name == description.name; });
    if(it != std::end(m_description.nativeFunctions))
        return;

    m_description.nativeFunctions[m_message_counter.functions++] = std::move(description);
    updateReadyness();
}

void ThymioNode::onEventDescriptionReceived(Aseba::LocalEventDescription description) {
    auto it = std::find_if(std::begin(m_description.localEvents), std::end(m_description.localEvents),
                           [&description](auto&& event) { return event.name == description.name; });
    if(it != std::end(m_description.localEvents))
        return;

    m_description.localEvents[m_message_counter.events++] = std::move(description);
    updateReadyness();
}

void ThymioNode::onUserMessageReceived(const Aseba::UserMessage& userMessage) {
    QList<int> data;
    data.reserve(int(userMessage.data.size()));
    std::copy(std::begin(userMessage.data), std::end(userMessage.data), std::back_inserter(data));
    Q_EMIT userMessageReceived(userMessage.type, data);
}

const ThymioProviderInfo& ThymioNode::provider() const {
    return m_provider;
}

uint16_t ThymioNode::id() const {
    return m_nodeId;
}

QString ThymioNode::name() const {
    if(provider().type() == ThymioProviderInfo::ProviderType::Serial ||
       provider().type() == ThymioProviderInfo::ProviderType::AndroidSerial) {
        return provider().name();
    }
    if(!m_description.name.empty()) {
        return QString::fromStdWString(m_description.name);
    }
    // This is so not useful !
    return QString::number(id());
}

bool ThymioNode::isReady() const {
    return isConnected() && m_ready;
}

bool ThymioNode::isConnected() const {
    return m_connection->isOpen();
}

void ThymioNode::updateReadyness() {
    if(m_ready)
        return;
    m_ready = !m_description.name.empty() && m_message_counter.variables == m_description.namedVariables.size() &&
        m_message_counter.events == m_description.localEvents.size() &&
        m_message_counter.functions == m_description.nativeFunctions.size();
    if(m_ready) {
        Q_EMIT ready();
    }
}

ThymioManager::ThymioManager(QObject* parent) : QObject(parent) {

// Thymio using the system-level serial driver only works on desktop OSes
#if(defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)) || defined(Q_OS_MAC) || defined(Q_OS_WIN)

    auto desktopProbe = new UsbSerialDeviceProber(this);
    connect(desktopProbe, &UsbSerialDeviceProber::availabilityChanged, this, &ThymioManager::scanDevices);
    m_probes.push_back(desktopProbe);

#endif

#ifdef Q_OS_ANDROID
    auto androidProbe = AndroidSerialDeviceProber::instance();
    connect(androidProbe, &AndroidSerialDeviceProber::availabilityChanged, this, &ThymioManager::scanDevices);
    m_probes.push_back(androidProbe);
#endif
    auto networkProbe = new NetworkDeviceProber(this);
    connect(networkProbe, &NetworkDeviceProber::availabilityChanged, this, &ThymioManager::scanDevices);
    m_probes.push_back(networkProbe);

    QTimer* t = new QTimer(this);
    connect(t, &QTimer::timeout, this, &ThymioManager::scanDevices);
    scanDevices();
    t->start(100);
}


void ThymioManager::scanDevices() {
    // Get the new providers
    std::vector<ThymioProviderInfo> new_providers;
    for(auto&& probe : m_probes) {
        auto thymios = probe->getDevices();
        std::move(std::begin(thymios), std::end(thymios), std::back_inserter(new_providers));
    }

    // Remove the one that disappeared
    for(auto providerIt = std::begin(m_providers); providerIt != std::end(m_providers);) {
        const auto& provider = *providerIt;

        if(std::find(std::begin(new_providers), std::end(new_providers), provider.first) != std::end(new_providers)) {
            providerIt++;
            continue;
        }
        providerIt = m_providers.erase(providerIt);

        // Remove the thymios associated to a given device
        auto it = std::remove_if(
            std::begin(m_thymios), std::end(m_thymios),
            [&provider](const std::shared_ptr<ThymioNode>& data) { return data->provider() == provider.first; });
        while(it != std::end(m_thymios)) {
            Robot r = std::move(*it);
            it = m_thymios.erase(it);
            Q_EMIT robotRemoved(r);
        }
    }

    // Register the new providers and open a connection to them
    for(auto&& provider : new_providers) {
        if(m_providers.find(provider) == m_providers.end()) {
            auto con = openConnection(provider);  // TODO : Manage errors ?
            if(!con)
                continue;
            m_providers.emplace(std::move(provider), con);
            connect(con.get(), &DeviceQtConnection::messageReceived, this, &ThymioManager::onMessageReceived);
        }
    }
    requestNodesList();
}  // namespace mobsya

std::shared_ptr<DeviceQtConnection> ThymioManager::openConnection(const ThymioProviderInfo& thymio) {
    for(auto&& probe : m_probes) {
        auto iod = probe->openConnection(thymio);
        if(iod)
            return iod;
    }
    return {};
}

void ThymioManager::requestNodesList() {
    std::for_each(std::begin(m_providers), std::end(m_providers), [](const auto& provider) {
        if(!provider.second) {
            return;
        }
        const std::shared_ptr<DeviceQtConnection>& connection = provider.second;
        connection->sendMessage(Aseba::ListNodes{});
    });
}

void ThymioManager::onMessageReceived(const ThymioProviderInfo& provider, std::shared_ptr<Aseba::Message> message) {

    auto it = std::find_if(std::begin(m_thymios), std::end(m_thymios),
                           [&provider, &message](const std::shared_ptr<ThymioNode>& node) {
                               return node->provider() == provider && node->id() == message->source;
                           });

    if(it == std::end(m_thymios) && message->type == ASEBA_MESSAGE_NODE_PRESENT) {
        auto providerIt = m_providers.find(provider);
        if(providerIt == std::end(m_providers))
            return;
        providerIt->second->sendMessage(Aseba::GetNodeDescription(message->source));
        it = m_thymios.insert(it, std::make_shared<ThymioNode>(providerIt->second, providerIt->first, message->source));
        Q_EMIT robotAdded(*it);
    }

    if(it != std::end(m_thymios)) {
        (*it)->onMessageReceived(message);
    }
}

const ThymioManager::Robot& ThymioManager::at(std::size_t index) const {
    return m_thymios.at(index);
}

ThymioManager::Robot ThymioManager::robotFromId(unsigned id) const {
    for(auto&& r : m_thymios)
        if(r->id() == id)
            return r;
    return {};
}

std::size_t ThymioManager::robotsCount() const {
    return m_thymios.size();
}

bool ThymioManager::hasDevice() const {
    return m_providers.size() > 0;
}

}  // namespace mobsya
