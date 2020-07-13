#include "thymionode.h"
#include "thymiodevicemanagerclientendpoint.h"
#include "qflatbuffers.h"
#include <range/v3/view/transform.hpp>
#include <range/v3/view/remove_if.hpp>
#include <QVersionNumber>

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

    connect(endpoint.get(), &ThymioDeviceManagerClientEndpoint::disconnected, this, [this] {
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

Q_INVOKABLE QObject* ThymioNode::endpoint_qml() const {
    return static_cast<QObject*>(m_endpoint.get());
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

QString ThymioNode::fwVersionAvailable() const {
    return m_fw_version_available;
}

QString ThymioNode::fwVersion() const {
    return m_fw_version;
}

ThymioNode::NodeCapabilities ThymioNode::capabilities() const {
    return m_capabilities;
}

bool ThymioNode::hasAvailableFirmwareUpdate() const {
    // Don't show update for remote thymios
    if(!m_endpoint || !m_endpoint->isLocalhostPeer())
        return false;
    const auto n = QVersionNumber::fromString(m_fw_version);
    return n < QVersionNumber::fromString(m_fw_version_available);
}

void ThymioNode::setCapabilities(const NodeCapabilities& capabilities) {
    if(m_capabilities != capabilities) {
        m_capabilities = capabilities;
        Q_EMIT capabilitiesChanged();
    }
}

bool ThymioNode::isInGroup() const {
    return !m_group || m_group->nodes().size() != 1;
}

void ThymioNode::setGroup(std::shared_ptr<ThymioGroup> group) {
    if(m_group)
        disconnect(m_group.get(), 0, this, 0);
    m_group = group;
    if(m_group)
        connect(m_group.get(), &ThymioGroup::groupChanged, this, &ThymioNode::groupChanged);
    Q_EMIT groupChanged();
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

CompilationRequest ThymioNode::save_aseba_code(const QByteArray& code) {
    return m_endpoint->save_code(*this, code, fb::ProgrammingLanguage::Aseba, fb::CompilationOptions::LoadOnTarget);
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
    return m_endpoint->set_watch_flags(m_uuid, int(m_watched_infos));
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

Request ThymioNode::setScratchPad(const QByteArray& data) {
    return m_endpoint->setScratchPad(uuid(), data, fb::ProgrammingLanguage::Aseba);
}

Request ThymioNode::upgradeFirmware() {
    return m_endpoint->upgradeFirmware(uuid());
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

void ThymioNode::onFirmwareUpgradeProgress(double d) {
    Q_EMIT firmwareUpdateProgress(d);
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

Q_INVOKABLE Request ThymioGroup::clearEventsAndVariables() {
    m_endpoint->setNodeEventsTable(m_group_id, {});
    return m_endpoint->setGroupVariables(*this, {});
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
    Q_EMIT groupChanged();
}

void ThymioGroup::removeNode(std::shared_ptr<ThymioNode> n) {
    for(auto it = m_nodes.begin(); it != m_nodes.end(); ++it) {
        if(auto ptr = it->lock(); !ptr || ptr == n) {
            m_nodes.erase(it);
            Q_EMIT groupChanged();
            return;
        }
    }
}

std::vector<std::shared_ptr<ThymioNode>> ThymioGroup::nodes() const {
    return m_nodes | ranges::view::transform([](auto&& node) { return node.lock(); }) |
        ranges::view::remove_if([](auto&& node) { return !node; }) | ranges::to_vector;
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
    } else if(!scratchpad.deleted) {
        it = m_scratchpads.insert(m_scratchpads.end(), scratchpad);
    }
    for(auto&& n : m_nodes) {
        if(auto t = n.lock(); t && t->uuid() == scratchpad.nodeId) {
            t->onScratchpadChanged(scratchpad.code, scratchpad.language);
        }
    }
    scratchPadChanged(scratchpad);
}

QVector<Scratchpad> ThymioGroup::scratchpads() const {
    return m_scratchpads;
}

Q_INVOKABLE Request ThymioGroup::loadAesl(const QByteArray& code) {
    return m_endpoint->send_aesl(*this, code);
}

void ThymioGroup::watchScratchpadsChanges(bool b) {
    if(m_watched_infos.testFlag(ThymioNode::WatchableInfo::Scratchpads) != b) {
        m_watched_infos.setFlag(ThymioNode::WatchableInfo::Scratchpads, b);
        m_endpoint->set_watch_flags(this->uuid(), m_watched_infos);
    }
}


}  // namespace mobsya
