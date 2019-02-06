#include "thymionode.h"
#include "thymiodevicemanagerclientendpoint.h"
#include "qflatbuffers.h"

namespace mobsya {

ThymioNode::ThymioNode(std::shared_ptr<ThymioDeviceManagerClientEndpoint> endpoint, const QUuid& uuid,
                       const QString& name, NodeType type, QObject* parent)
    : QObject(parent)
    , m_endpoint(endpoint)
    , m_uuid(uuid)
    , m_name(name)
    , m_status(Status::Disconnected)
    , m_executionState(VMExecutionState::Stopped)
    , m_type(type) {

    connect(endpoint.get(), &ThymioDeviceManagerClientEndpoint::disconnected, [this] {
        m_status = Status::Disconnected;
        Q_EMIT statusChanged();
    });
    connect(this, &ThymioNode::groupChanged, this, &ThymioNode::modified);
    connect(this, &ThymioNode::nameChanged, this, &ThymioNode::modified);
    connect(this, &ThymioNode::statusChanged, this, &ThymioNode::modified);
    connect(this, &ThymioNode::capabilitiesChanged, this, &ThymioNode::modified);
}


QUuid ThymioNode::uuid() const {
    return m_uuid;
}

QUuid ThymioNode::group_id() const {
    if(!m_group)
        return {};
    return m_group->uuid();
}

std::shared_ptr<ThymioGroup> ThymioNode::group() const {
    return m_group;
}

QString ThymioNode::name() const {
    return m_name;
}

ThymioNode::Status ThymioNode::status() const {
    return m_status;
}

ThymioNode::VMExecutionState ThymioNode::vmExecutionState() const {
    return m_executionState;
}

ThymioNode::NodeType ThymioNode::type() const {
    return m_type;
}

QUrl ThymioNode::websocketEndpoint() const {
    return m_endpoint->websocketConnectionUrl();
}

ThymioNode::NodeCapabilities ThymioNode::capabilities() {
    return m_capabilities;
}

void ThymioNode::setCapabilities(const NodeCapabilities& capabilities) {
    if(m_capabilities != capabilities) {
        m_capabilities = capabilities;
        Q_EMIT capabilitiesChanged();
    }
}

void ThymioNode::setGroup(std::shared_ptr<ThymioGroup> group) {
    m_group = group;
}

void ThymioNode::setName(const QString& name) {
    if(m_name != name) {
        m_name = name;
        Q_EMIT nameChanged();
    }
}

void ThymioNode::setStatus(const Status& status) {
    if(m_status != status) {
        m_status = status;
        Q_EMIT statusChanged();
    }
}

Request ThymioNode::rename(const QString& newName) {
    if(newName != m_name) {
        Q_EMIT nameChanged();
        return m_endpoint->renameNode(*this, newName);
    }
    return Request();
}

Request ThymioNode::lock() {
    return m_endpoint->lock(*this);
}

Request ThymioNode::unlock() {
    return m_endpoint->unlock(*this);
}

Request ThymioNode::stop() {
    return m_endpoint->setNodeExecutionState(*this, fb::VMExecutionStateCommand::Stop);
}

Request ThymioNode::run() {
    return m_endpoint->setNodeExecutionState(*this, fb::VMExecutionStateCommand::Run);
}

Request ThymioNode::pause() {
    return m_endpoint->setNodeExecutionState(*this, fb::VMExecutionStateCommand::Pause);
}
Request ThymioNode::reboot() {
    return m_endpoint->setNodeExecutionState(*this, fb::VMExecutionStateCommand::Reboot);
}

Request ThymioNode::step() {
    return m_endpoint->setNodeExecutionState(*this, fb::VMExecutionStateCommand::Step);
}

Request ThymioNode::stepToNextLine() {
    return m_endpoint->setNodeExecutionState(*this, fb::VMExecutionStateCommand::StepToNextLine);
}

Request ThymioNode::writeProgramToDeviceMemory() {
    return m_endpoint->setNodeExecutionState(*this, fb::VMExecutionStateCommand::WriteProgramToDeviceMemory);
}

BreakpointsRequest ThymioNode::setBreakPoints(const QVector<unsigned>& breakpoints) {
    return m_endpoint->setNodeBreakPoints(*this, breakpoints);
}

CompilationRequest ThymioNode::compile_aseba_code(const QByteArray& code) {
    return m_endpoint->send_code(*this, code, fb::ProgrammingLanguage::Aseba, fb::CompilationOptions(0));
}

CompilationRequest ThymioNode::load_aseba_code(const QByteArray& code) {
    return m_endpoint->send_code(*this, code, fb::ProgrammingLanguage::Aseba, fb::CompilationOptions::LoadOnTarget);
}

Request ThymioNode::setWatchVariablesEnabled(bool enabled) {
    m_watched_infos.setFlag(WatchableInfo::Variables, enabled);
    return updateWatchedInfos();
}

Request ThymioNode::setWatchEventsEnabled(bool enabled) {
    m_watched_infos.setFlag(WatchableInfo::Events, enabled);
    return updateWatchedInfos();
}

Request ThymioNode::setWatchVMExecutionStateEnabled(bool enabled) {
    m_watched_infos.setFlag(WatchableInfo::VMExecutionState, enabled);
    return updateWatchedInfos();
}

Request ThymioNode::setWatchSharedVariablesEnabled(bool enabled) {
    m_watched_infos.setFlag(WatchableInfo::SharedVariables, enabled);
    return updateWatchedInfos();
}
Request ThymioNode::setWatchEventsDescriptionEnabled(bool enabled) {
    m_watched_infos.setFlag(WatchableInfo::SharedEventsDescription, enabled);
    return updateWatchedInfos();
}

Request ThymioNode::updateWatchedInfos() {
    return m_endpoint->set_watch_flags(*this, int(m_watched_infos));
}

AsebaVMDescriptionRequest ThymioNode::fetchAsebaVMDescription() {
    return m_endpoint->fetchAsebaVMDescription(*this);
}

Request ThymioNode::setVariables(const VariableMap& variables) {
    return m_endpoint->setNodeVariables(*this, variables);
}

Request ThymioNode::setGroupVariables(const VariableMap& variables) {
    return m_group->setSharedVariables(variables);
}

Request ThymioNode::addEvent(const EventDescription& d) {
    return m_group->addEvent(d);
}

Request ThymioNode::removeEvent(const QString& name) {
    return m_group->removeEvent(name);
}

Request ThymioNode::emitEvent(const QString& name, const QVariant& value) {
    EventMap map;
    map.insert(name, value);
    return m_endpoint->emitNodeEvents(*this, map);
}


void ThymioNode::onExecutionStateChanged(const fb::VMExecutionStateChangedT& msg) {
    if(msg.error == fb::VMExecutionError::NoError) {
        m_executionState = msg.state;
        Q_EMIT vmExecutionStateChanged();
        if(m_executionState == VMExecutionState::Paused) {
            Q_EMIT vmExecutionPaused(msg.line);
        } else if(m_executionState == VMExecutionState::Running) {
            Q_EMIT vmExecutionStarted();
        } else if(m_executionState == VMExecutionState::Stopped) {
            Q_EMIT vmExecutionStopped();
        }
    } else {
        Q_EMIT vmExecutionStopped();
        Q_EMIT vmExecutionError(msg.error, QString::fromStdString(msg.errorMsg), msg.line);
    }
}

void ThymioNode::onVariablesChanged(VariableMap variables, const QDateTime& timestamp) {
    Q_EMIT variablesChanged(variables, timestamp);
}

void ThymioNode::onGroupVariablesChanged(VariableMap variables, const QDateTime& timestamp) {
    Q_EMIT groupVariablesChanged(variables, timestamp);
}

void ThymioNode::onScratchpadChanged(const QString& text, fb::ProgrammingLanguage language) {
    Q_EMIT scratchpadChanged(text, language);
}


void ThymioNode::onEvents(const EventMap& evs, const QDateTime& timestamp) {
    Q_EMIT events(evs, timestamp);
}

void ThymioNode::onEventsTableChanged(const QVector<EventDescription>& events) {
    Q_EMIT eventsTableChanged(events);
}

ThymioGroup::ThymioGroup(std::shared_ptr<ThymioDeviceManagerClientEndpoint> endpoint, const QUuid& id)
    : m_group_id(id), m_endpoint(endpoint) {}

QUuid ThymioGroup::uuid() const {
    return m_group_id;
}

Request ThymioGroup::setSharedVariables(const VariableMap& variables) {
    return m_endpoint->setGroupVariables(*this, variables);
}

Request ThymioGroup::addEvent(const EventDescription& d) {
    auto& table = m_events_table;
    auto it =
        std::find_if(table.begin(), table.end(), [&d](const EventDescription& ed) { return d.name() == ed.name(); });
    if(it != table.end()) {
        *it = d;
    } else {
        table.append(d);
    }
    return m_endpoint->setNodeEventsTable(m_group_id, table);
}

Request ThymioGroup::removeEvent(const QString& name) {
    auto table = m_events_table;
    auto it =
        std::find_if(table.begin(), table.end(), [&name](const EventDescription& ed) { return name == ed.name(); });
    if(it != table.end()) {
        table.erase(it);
    }
    return m_endpoint->setNodeEventsTable(m_group_id, table);
}

QVector<EventDescription> ThymioGroup::eventsDescriptions() const {
    return m_events_table;
}

ThymioGroup::VariableMap ThymioGroup::sharedVariables() const {
    return m_shared_variables;
}

void ThymioGroup::addNode(std::shared_ptr<ThymioNode> n) {
    for(auto&& o : m_nodes) {
        if(auto thymio = o.lock(); n == thymio)
            return;
    }
    m_nodes.push_back(n);
}

void ThymioGroup::removeNode(std::shared_ptr<ThymioNode> n) {
    for(auto it = m_nodes.begin(); it != m_nodes.end(); ++it) {
        if(it->lock() == n) {
            m_nodes.erase(it);
            return;
        }
    }
}

void ThymioGroup::onSharedVariablesChanged(VariableMap variables, const QDateTime& timestamp) {
    m_shared_variables = variables;
    for(auto&& n : m_nodes) {
        if(auto t = n.lock())
            t->onGroupVariablesChanged(variables, timestamp);
    }
}

void ThymioGroup::onEventsDescriptionsChanged(const QVector<EventDescription>& events) {
    m_events_table = events;
    for(auto&& n : m_nodes) {
        if(auto t = n.lock())
            t->onEventsTableChanged(events);
    }
}

void ThymioGroup::onScratchpadChanged(const Scratchpad& scratchpad) {
    auto it = std::find_if(m_scratchpads.begin(), m_scratchpads.end(),
                           [&scratchpad](auto&& s) { return s.id == scratchpad.id; });
    if(it != m_scratchpads.end()) {
        *it = scratchpad;
    } else {
        it = m_scratchpads.insert(m_scratchpads.end(), scratchpad);
    }
    for(auto&& n : m_nodes) {
        if(auto t = n.lock(); t->uuid() == it->nodeId) {
            t->onScratchpadChanged(it->code, it->language);
        }
    }
}

Q_INVOKABLE Request ThymioGroup::loadAesl(const QByteArray& code) {
    return m_endpoint->send_aesl(*this, code);
}


}  // namespace mobsya
