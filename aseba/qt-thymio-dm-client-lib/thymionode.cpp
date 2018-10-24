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

    connect(this, &ThymioNode::nameChanged, this, &ThymioNode::modified);
    connect(this, &ThymioNode::statusChanged, this, &ThymioNode::modified);
    connect(this, &ThymioNode::capabilitiesChanged, this, &ThymioNode::modified);
}


QUuid ThymioNode::uuid() const {
    return m_uuid;
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

Request ThymioNode::updateWatchedInfos() {
    return m_endpoint->set_watch_flags(*this, int(m_watched_infos));
}

AsebaVMDescriptionRequest ThymioNode::fetchAsebaVMDescription() {
    return m_endpoint->fetchAsebaVMDescription(*this);
}

Request ThymioNode::setVariabes(const VariableMap& variables) {
    return m_endpoint->setNodeVariabes(*this, variables);
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
    }
}

void ThymioNode::onVariablesChanged(VariableMap variables) {
    Q_EMIT variablesChanged(variables);
}

void ThymioNode::onEvents(VariableMap evs) {
    Q_EMIT events(evs);
}


}  // namespace mobsya
