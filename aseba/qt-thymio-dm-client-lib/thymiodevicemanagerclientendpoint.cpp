#include "thymiodevicemanagerclientendpoint.h"
#include <QDataStream>
#include <QtEndian>
#include <array>
#include <QtGlobal>
#include <QDateTime>
#include <QRandomGenerator>
#include "qflatbuffers.h"
#include "thymio-api.h"

namespace mobsya {

ThymioDeviceManagerClientEndpoint::ThymioDeviceManagerClientEndpoint(QTcpSocket* socket, QString host, QObject* parent)
    : QObject(parent)
    , m_socket(socket)
    , m_message_size(0)
    , m_host_name(std::move(host))
    , m_socket_health_check_timer(new QTimer(this))
    , m_dongles_manager(new Thymio2WirelessDonglesManager(this)) {

    m_socket->setParent(this);
    connect(m_socket, &QTcpSocket::readyRead, this, &ThymioDeviceManagerClientEndpoint::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &ThymioDeviceManagerClientEndpoint::disconnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &ThymioDeviceManagerClientEndpoint::cancelAllRequests);
    connect(m_socket, &QTcpSocket::connected, this, &ThymioDeviceManagerClientEndpoint::onConnected);
    connect(m_socket, qOverload<QAbstractSocket::SocketError>(&QAbstractSocket::error), this,
            &ThymioDeviceManagerClientEndpoint::cancelAllRequests);

    // The TDM pings every 25000ms
    m_socket_health_check_timer->start(15000);
    connect(m_socket_health_check_timer, &QTimer::timeout, this, &ThymioDeviceManagerClientEndpoint::checkSocketHealth);


    connect(this, &ThymioDeviceManagerClientEndpoint::nodeAdded, this,
            &ThymioDeviceManagerClientEndpoint::nodesChanged);
    connect(this, &ThymioDeviceManagerClientEndpoint::nodeRemoved, this,
            &ThymioDeviceManagerClientEndpoint::nodesChanged);
}

ThymioDeviceManagerClientEndpoint::~ThymioDeviceManagerClientEndpoint() {
    cancelAllRequests();
}

void ThymioDeviceManagerClientEndpoint::write(const flatbuffers::DetachedBuffer& buffer) {
    uint32_t size = uint32_t(buffer.size());
    m_socket->write(reinterpret_cast<const char*>(&size), sizeof(size));
    m_socket->write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
    m_socket->flush();
}

void ThymioDeviceManagerClientEndpoint::write(const tagged_detached_flatbuffer& buffer) {
    write(buffer.buffer);
}


QHostAddress ThymioDeviceManagerClientEndpoint::peerAddress() const {
    return m_socket->peerAddress();
}

QString ThymioDeviceManagerClientEndpoint::hostName() const {
    return m_host_name;
}

void ThymioDeviceManagerClientEndpoint::setWebSocketMatchingPort(quint16 port) {
    m_ws_port = port;
}

QByteArray ThymioDeviceManagerClientEndpoint::password() const {
    return m_password;
}

void ThymioDeviceManagerClientEndpoint::setPassword(QByteArray password) {
    m_password = password;
    Q_EMIT passwordChanged();
}

QUrl ThymioDeviceManagerClientEndpoint::websocketConnectionUrl() const {
    QUrl u;
    if(!m_ws_port)
        return {};
    u.setScheme("ws");
    u.setHost(m_socket->peerAddress().toString());
    u.setPort(m_ws_port);
    return u;
}

QUrl ThymioDeviceManagerClientEndpoint::tcpConnectionUrl() const {
    QUrl u;
    u.setScheme("tcp");
    u.setHost(m_socket->peerAddress().toString());
    u.setPort(m_socket->peerPort());
    return u;
}

Thymio2WirelessDonglesManager* ThymioDeviceManagerClientEndpoint::donglesManager() const {
    return m_dongles_manager;
}

void ThymioDeviceManagerClientEndpoint::checkSocketHealth() {
    if(m_last_message_reception_date.msecsTo(QDateTime::currentDateTime()) > 10 * 1000) {
        qWarning() << "Connection timed out";
        // m_socket->disconnectFromHost();
        disconnect(m_socket_health_check_timer);
    }
}

void ThymioDeviceManagerClientEndpoint::onReadyRead() {
    if(m_message_size == 0) {
        if(m_socket->bytesAvailable() < 4)
            return;
        char data[4];
        m_socket->read(data, 4);
        m_message_size = *reinterpret_cast<quint32*>(data);
    }
    if(m_socket->bytesAvailable() < m_message_size)
        return;
    std::vector<uint8_t> data;
    data.resize(m_message_size);
    auto s = m_socket->read(reinterpret_cast<char*>(data.data()), m_message_size);
    Q_ASSERT(s == m_message_size);
    m_message_size = 0;
    flatbuffers::Verifier verifier(data.data(), data.size());
    if(!fb::VerifyMessageBuffer(verifier)) {
        qWarning() << "Invalid incomming message";
    }
    handleIncommingMessage(fb_message_ptr(std::move(data)));
    QMetaObject::invokeMethod(this, "onReadyRead", Qt::QueuedConnection);
}

std::shared_ptr<ThymioNode> ThymioDeviceManagerClientEndpoint::node(const QUuid& id) const {
    return m_nodes.value(id);
}

void ThymioDeviceManagerClientEndpoint::handleIncommingMessage(const fb_message_ptr& msg) {
    qDebug() << "ThymioDeviceManagerClientEndpoint::handleIncommingMessage" << EnumNameAnyMessage(msg.message_type());
    m_last_message_reception_date = QDateTime::currentDateTime();
    switch(msg.message_type()) {
        case mobsya::fb::AnyMessage::ConnectionHandshake: {
            auto message = msg.as<mobsya::fb::ConnectionHandshake>()->UnPack();
            m_islocalhostPeer = message->localhostPeer;
            localPeerChanged();
            m_serverUUid = message->uuid ? qfb::uuid(message->uuid->id) : QUuid{};
            m_ws_port = message->ws_port;
            setPassword(QByteArray::fromStdString(message->password));
            handshakeCompleted(m_serverUUid);
            // TODO handle versionning, etc
            break;
        }
        case mobsya::fb::AnyMessage::NodesChanged: {
            auto message = msg.as<mobsya::fb::NodesChanged>();
            onNodesChanged(*(message->UnPack()));
            break;
        }
        case mobsya::fb::AnyMessage::SaveBytecode: {
            auto message = msg.as<mobsya::fb::SaveBytecode>();
            auto basic_req = get_request(message->request_id());
            
            if(!basic_req)
                break;

            auto id = qfb::uuid(message->node_id()->UnPack());
            if(auto node = m_nodes.value(id)) {
                node->ReadyBytecode(message->program()->c_str());
            }
            break;
        }
        case mobsya::fb::AnyMessage::RequestCompleted: {
            auto message = msg.as<mobsya::fb::RequestCompleted>();
            auto basic_req = get_request(message->request_id());
            if(!basic_req)
                break;
            if(auto req = basic_req->as<Request::internal_ptr_type>()) {
                req->setResult();
            }
            break;
        }
        case mobsya::fb::AnyMessage::Error: {
            auto message = msg.as<mobsya::fb::Error>();
            auto basic_req = get_request(message->request_id());
            if(!basic_req)
                break;
            basic_req->setError(message->error());
            break;
        }
        case mobsya::fb::AnyMessage::CompilationResultFailure: {
            auto message = msg.as<mobsya::fb::CompilationResultFailure>();
            auto basic_req = get_request(message->request_id());
            if(!basic_req)
                break;
            if(auto req = basic_req->as<CompilationRequest::internal_ptr_type>()) {
                auto r = CompilationResult::make_error(QString(message->message()->c_str()), message->character(),
                                                       message->line(), message->column());
                req->setResult(std::move(r));
            }
            break;
        }
        case mobsya::fb::AnyMessage::CompilationResultSuccess: {
            auto message = msg.as<mobsya::fb::CompilationResultSuccess>();
            auto basic_req = get_request(message->request_id());
            if(!basic_req)
                break;
            if(auto req = basic_req->as<CompilationRequest::internal_ptr_type>()) {
                auto r =
                    CompilationResult::make_success(message->bytecode_size(), message->variables_size(),
                                                    message->total_bytecode_size(), message->total_variables_size());
                req->setResult(std::move(r));
            }
            break;
        }

        case mobsya::fb::AnyMessage::SetBreakpointsResponse: {
            auto message = msg.as<mobsya::fb::SetBreakpointsResponse>();
            auto basic_req = get_request(message->request_id());
            if(!basic_req)
                break;
            if(auto req = basic_req->as<BreakpointsRequest::internal_ptr_type>()) {
                const fb::SetBreakpointsResponseT& r = *(message->UnPack());
                QVector<unsigned> breakpoints;
                for(const auto& bp : r.breakpoints) {
                    if(bp)
                        breakpoints.append(bp->line);
                }
                req->setResult(SetBreakpointRequestResult(breakpoints));
            }
            break;
        }

        case mobsya::fb::AnyMessage::VMExecutionStateChanged: {
            auto message = msg.as<mobsya::fb::VMExecutionStateChanged>()->UnPack();
            if(!message)
                break;
            auto id = qfb::uuid(message->node_id->id);
            if(auto node = m_nodes.value(id)) {
                node->onExecutionStateChanged(*message);
            }
            break;
        }

        case mobsya::fb::AnyMessage::VariablesChanged: {
            auto message = msg.as<mobsya::fb::VariablesChanged>();
            if(!message || !message->vars())
                break;
            auto id = qfb::uuid(message->node_id()->UnPack());
            const auto time = message->timestamp() == 0 ? QDateTime::currentDateTime() :
                                                          QDateTime::fromMSecsSinceEpoch(message->timestamp());

            ThymioNode::VariableMap vars;
            for(const auto& var : *(message->vars())) {
                if(!var)
                    continue;
                auto name = qfb::as_qstring(var->name());
                if(name.isEmpty())
                    continue;
                auto value = qfb::to_qvariant(var->value_flexbuffer_root());
                vars.insert(name, ThymioVariable(value));
            }
            if(auto grp = group_from_group_id(id)) {
                grp->onSharedVariablesChanged(vars, time);
            } else {
                for(auto&& node : nodes(id)) {
                    node->onVariablesChanged(vars, time);
                }
            }
            break;
        }

        case mobsya::fb::AnyMessage::EventsEmitted: {
            auto message = msg.as<mobsya::fb::EventsEmitted>();
            if(!message || !message->events())
                break;
            auto id = qfb::uuid(message->node_id()->UnPack());
            auto node = m_nodes.value(id);
            if(!node)
                break;
            ThymioNode::EventMap events;
            for(const auto& event : *(message->events())) {
                if(!event)
                    continue;
                auto name = qfb::as_qstring(event->name());
                if(name.isEmpty())
                    continue;
                auto value = qfb::to_qvariant(event->value_flexbuffer_root());
                events.insert(name, value);
            }
            const auto time = message->timestamp() == 0 ? QDateTime::currentDateTime() :
                                                          QDateTime::fromMSecsSinceEpoch(message->timestamp());
            node->onEvents(std::move(events), time);
            break;
        }

        case mobsya::fb::AnyMessage::EventsDescriptionsChanged: {
            auto message = msg.as<mobsya::fb::EventsDescriptionsChanged>();
            if(!message || !message->events())
                break;
            auto id = qfb::uuid(message->node_or_group_id()->UnPack());
            auto grp = this->group_from_id(id);
            if(!grp)
                break;
            QVector<mobsya::EventDescription> events;
            for(const auto& event : *(message->events())) {
                if(!event)
                    continue;
                auto name = qfb::as_qstring(event->name());
                if(name.isEmpty())
                    continue;
                auto size = event->fixed_sized();
                events.append(EventDescription(name, size));
            }
            grp->onEventsDescriptionsChanged(events);
            break;
        }

        case mobsya::fb::AnyMessage::NodeAsebaVMDescription: {
            auto message = msg.as<mobsya::fb::NodeAsebaVMDescription>();
            if(!message)
                break;
            auto basic_req = get_request(message->request_id());
            if(!basic_req)
                break;
            auto req = basic_req->as<AsebaVMDescriptionRequest::internal_ptr_type>();
            if(!req)
                break;
            const fb::NodeAsebaVMDescriptionT& r = *(message->UnPack());
            req->setResult(AsebaVMDescriptionRequestResult(r));
            break;
        }
        case mobsya::fb::AnyMessage::ScratchpadUpdate: {
            auto message = msg.as<mobsya::fb::ScratchpadUpdate>();
            auto groupid = qfb::uuid(message->group_id()->UnPack());
            auto grp = this->group_from_id(groupid);
            if(!grp)
                break;

            Scratchpad pad;
            pad.id = qfb::uuid(message->scratchpad_id()->UnPack());
            pad.nodeId = qfb::uuid(message->node_id()->UnPack());
            pad.code = qfb::as_qstring(message->text());
            pad.language = message->language();
            pad.name = qfb::as_qstring(message->name());
            pad.deleted = message->deleted();
            grp->onScratchpadChanged(pad);
            break;
        }
        case mobsya::fb::AnyMessage::FirmwareUpgradeStatus: {
            auto message = msg.as<mobsya::fb::FirmwareUpgradeStatus>();
            if(!message)
                break;
            auto id = qfb::uuid(message->node_id()->UnPack());
            auto node = m_nodes.value(id);
            if(!node)
                break;
            node->onFirmwareUpgradeProgress(message->progress());
            break;
        }
        case mobsya::fb::AnyMessage::Thymio2WirelessDonglesChanged: {
            auto message = msg.as<mobsya::fb::Thymio2WirelessDonglesChanged>();
            if(!message || !message->dongles())
                break;
            m_dongles_manager->clear();
            for(auto&& d : *message->dongles()) {
                const auto& v = d->UnPack();
                auto dongleId = qfb::uuid(*v->dongle_id);

                m_dongles_manager->updateDongle(
                    Thymio2WirelessDongle{dongleId, v->network_id, v->dongle_node, v->channel_id});
            }
            break;
        }
        case mobsya::fb::AnyMessage::Thymio2WirelessDonglePairingResponse: {
            auto message = msg.as<mobsya::fb::Thymio2WirelessDonglePairingResponse>();
            auto basic_req = get_request(message->request_id());
            if(!basic_req)
                break;
            if(auto req = basic_req->as<Thymio2WirelessDonglePairingRequest::internal_ptr_type>()) {
                req->setResult(Thymio2WirelessDonglePairingResult(message->network_id(), message->channel_id()));
            }
            break;
        }


        default: Q_EMIT onMessage(msg);
    }
}

void ThymioDeviceManagerClientEndpoint::onNodesChanged(const fb::NodesChangedT& nodes) {
    for(const auto& ptr : nodes.nodes) {
        const fb::NodeT& node = *ptr;
        const QUuid id = qfb::uuid(*node.node_id);
        const QUuid group_id = node.group_id ? qfb::uuid(*node.group_id) : QUuid{};
        if(id.isNull())
            continue;
        ThymioNode::NodeCapabilities caps;
        if(node.capabilities & uint64_t(fb::NodeCapability::ForceResetAndStop))
            caps |= ThymioNode::NodeCapability::ForceResetAndStop;
        if(node.capabilities & uint64_t(fb::NodeCapability::Rename))
            caps |= ThymioNode::NodeCapability::Rename;
        if(node.capabilities & uint64_t(fb::NodeCapability::FirwmareUpgrade))
            caps |= ThymioNode::NodeCapability::FirmwareUpgrade;
        QString name = QString::fromStdString(node.name);
        auto status = static_cast<ThymioNode::Status>(node.status);
        auto type = static_cast<ThymioNode::NodeType>(node.type);


        bool added = false;
        auto it = m_nodes.find(id);
        if(it == m_nodes.end()) {
            it = m_nodes.insert(id,
                                std::shared_ptr<ThymioNode>(new ThymioNode(shared_from_this(), id, name, type),
                                                            [](ThymioNode* n) { n->deleteLater(); }));
            added = true;
        }

        (*it)->setName(name);
        (*it)->setStatus(status);
        (*it)->setCapabilities(caps);

        if(auto old_group = (*it)->group()) {
            old_group->removeNode(*it);
        }
        auto new_grp = group_from_group_id(group_id);
        if(!new_grp) {
            new_grp = std::shared_ptr<ThymioGroup>(new ThymioGroup(shared_from_this(), group_id),
                                                   [](ThymioGroup* g) { g->deleteLater(); });
        }
        new_grp->addNode(*it);
        (*it)->setGroup(new_grp);
        (*it)->m_fw_version = QString::fromStdString(node.fw_version);
        (*it)->m_fw_version_available = QString::fromStdString(node.latest_fw_version);

        Q_EMIT added ? nodeAdded(it.value()) : nodeModified(it.value());

        if(status == ThymioNode::Status::Disconnected) {
            auto node = it.value();
            node->setGroup({});
            m_nodes.erase(it);
            Q_EMIT nodeRemoved(node);
        }
    }
}

detail::RequestDataBase::shared_ptr
ThymioDeviceManagerClientEndpoint::get_request(detail::RequestDataBase::request_id id) {
    auto it = m_pending_requests.find(id);
    if(it == m_pending_requests.end())
        return {};
    auto res = *it;
    m_pending_requests.erase(it);
    return res;
}

void ThymioDeviceManagerClientEndpoint::cancelAllRequests() {
    for(auto&& r : m_pending_requests) {
        r->cancel();
    }
    m_pending_requests.clear();
}

bool ThymioDeviceManagerClientEndpoint::isLocalhostPeer() const {
    return m_islocalhostPeer;
}

Request ThymioDeviceManagerClientEndpoint::requestDeviceManagerShutdown() {
    Request r = prepare_request<Request>();
    flatbuffers::FlatBufferBuilder builder;
    fb::DeviceManagerShutdownRequestBuilder msg(builder);
    msg.add_request_id(r.id());
    write(wrap_fb(builder, msg.Finish()));
    return r;
}

void ThymioDeviceManagerClientEndpoint::onConnected() {

    flatbuffers::Offset<flatbuffers::String> passwordOffset;
    flatbuffers::FlatBufferBuilder builder;
    if(!m_password.isEmpty())
        passwordOffset = qfb::add_string(builder, m_password);

    fb::ConnectionHandshakeBuilder hsb(builder);
    hsb.add_protocolVersion(protocolVersion);
    hsb.add_minProtocolVersion(minProtocolVersion);
    hsb.add_password(passwordOffset);
    write(wrap_fb(builder, hsb.Finish()));
}

flatbuffers::Offset<fb::NodeId> ThymioDeviceManagerClientEndpoint::serialize_uuid(flatbuffers::FlatBufferBuilder& fb,
                                                                                  const QUuid& uuid) {
    std::array<uint8_t, 16> data;
    *reinterpret_cast<uint32_t*>(data.data()) = qFromBigEndian(uuid.data1);
    *reinterpret_cast<uint16_t*>(data.data() + 4) = qFromBigEndian(uuid.data2);
    *reinterpret_cast<uint16_t*>(data.data() + 6) = qFromBigEndian(uuid.data3);
    memcpy(data.data() + 8, uuid.data4, 8);

    return mobsya::fb::CreateNodeId(fb, fb.CreateVector(data.data(), data.size()));
}

Request ThymioDeviceManagerClientEndpoint::renameNode(const ThymioNode& node, const QString& newName) {

    Request r = prepare_request<Request>();
    flatbuffers::FlatBufferBuilder builder;
    auto uuidOffset = serialize_uuid(builder, node.uuid());
    auto nameOffset = qfb::add_string(builder, newName);
    fb::RenameNodeBuilder rn(builder);
    rn.add_request_id(r.id());
    rn.add_node_id(uuidOffset);
    rn.add_new_name(nameOffset);
    write(wrap_fb(builder, rn.Finish()));
    return r;
}

Request ThymioDeviceManagerClientEndpoint::setNodeExecutionState(const ThymioNode& node,
                                                                 fb::VMExecutionStateCommand cmd) {
    Request r = prepare_request<Request>();
    flatbuffers::FlatBufferBuilder builder;
    auto uuidOffset = serialize_uuid(builder, node.uuid());
    write(wrap_fb(builder, fb::CreateSetVMExecutionState(builder, r.id(), uuidOffset, cmd)));
    return r;
}

BreakpointsRequest ThymioDeviceManagerClientEndpoint::setNodeBreakPoints(const ThymioNode& node,
                                                                         const QVector<unsigned>& breakpoints) {
    BreakpointsRequest r = prepare_request<BreakpointsRequest>();
    flatbuffers::FlatBufferBuilder builder;
    auto uuidOffset = serialize_uuid(builder, node.uuid());
    std::vector<flatbuffers::Offset<fb::Breakpoint>> serialized_breakpoints;
    serialized_breakpoints.reserve(breakpoints.size());
    std::transform(breakpoints.begin(), breakpoints.end(), std::back_inserter(serialized_breakpoints),
                   [&builder](const auto bp) { return fb::CreateBreakpoint(builder, bp); });
    auto vecOffset = builder.CreateVector(serialized_breakpoints);
    write(wrap_fb(builder, fb::CreateSetBreakpoints(builder, r.id(), uuidOffset, vecOffset)));
    return r;
}

auto ThymioDeviceManagerClientEndpoint::lock(const ThymioNode& node) -> Request {
    Request r = prepare_request<Request>();
    flatbuffers::FlatBufferBuilder builder;
    auto uuidOffset = serialize_uuid(builder, node.uuid());
    write(wrap_fb(builder, fb::CreateLockNode(builder, r.id(), uuidOffset)));
    return r;
}
auto ThymioDeviceManagerClientEndpoint::unlock(const ThymioNode& node) -> Request {
    Request r = prepare_request<Request>();
    flatbuffers::FlatBufferBuilder builder;
    auto uuidOffset = serialize_uuid(builder, node.uuid());
    write(wrap_fb(builder, fb::CreateUnlockNode(builder, r.id(), uuidOffset)));
    return r;
}

auto ThymioDeviceManagerClientEndpoint::send_code(const ThymioNode& node, const QByteArray& code,
                                                  fb::ProgrammingLanguage language, fb::CompilationOptions opts)
    -> CompilationRequest {

    CompilationRequest r = prepare_request<CompilationRequest>();
    flatbuffers::FlatBufferBuilder builder;
    auto uuidOffset = serialize_uuid(builder, node.uuid());
    auto codedOffset = builder.CreateString(code.data(), code.size());
    write(wrap_fb(builder, fb::CreateCompileAndLoadCodeOnVM(builder, r.id(), uuidOffset, language, codedOffset, opts)));
    return r;
}
auto ThymioDeviceManagerClientEndpoint::save_code(const ThymioNode& node, const QByteArray& code,
                                                  fb::ProgrammingLanguage language, fb::CompilationOptions opts)
    -> CompilationRequest {

    CompilationRequest r = prepare_request<CompilationRequest>();
    flatbuffers::FlatBufferBuilder builder;
    auto uuidOffset = serialize_uuid(builder, node.uuid());
    auto codedOffset = builder.CreateString(code.data(), code.size());
    write(wrap_fb(builder, fb::CreateCompileAndSave(builder, r.id(), uuidOffset, language, codedOffset, opts)));
    
    return r;
}


Request ThymioDeviceManagerClientEndpoint::send_aesl(const ThymioGroup& group, const QByteArray& code) {
    auto r = prepare_request<Request>();
    flatbuffers::FlatBufferBuilder builder;
    auto uuidOffset = serialize_uuid(builder, group.uuid());
    auto codedOffset = builder.CreateString(code.data(), code.size());
    write(wrap_fb(
        builder,
        fb::CreateCompileAndLoadCodeOnVM(builder, r.id(), uuidOffset, fb::ProgrammingLanguage::Aesl, codedOffset)));
    return r;
}

Request ThymioDeviceManagerClientEndpoint::set_watch_flags(const QUuid& node, int flags) {
    Request r = prepare_request<Request>();
    flatbuffers::FlatBufferBuilder builder;
    auto uuidOffset = serialize_uuid(builder, node);
    write(wrap_fb(builder, fb::CreateWatchNode(builder, r.id(), uuidOffset, flags)));
    return r;
}

AsebaVMDescriptionRequest ThymioDeviceManagerClientEndpoint::fetchAsebaVMDescription(const ThymioNode& node) {
    AsebaVMDescriptionRequest r = prepare_request<AsebaVMDescriptionRequest>();
    flatbuffers::FlatBufferBuilder builder;
    auto uuidOffset = serialize_uuid(builder, node.uuid());
    write(wrap_fb(builder, fb::CreateRequestNodeAsebaVMDescription(builder, r.id(), uuidOffset)));
    return r;
}

namespace detail {
    auto serialize_variables(flatbuffers::FlatBufferBuilder& fb, const ThymioNode::VariableMap& vars) {
        flexbuffers::Builder flexbuilder;
        std::vector<flatbuffers::Offset<fb::NodeVariable>> varsOffsets;
        varsOffsets.reserve(vars.size());
        for(auto&& var : vars.toStdMap()) {
            qfb::to_flexbuffer(var.second.value(), flexbuilder);
            auto& vec = flexbuilder.GetBuffer();
            auto vecOffset = fb.CreateVector(vec);
            auto keyOffset = qfb::add_string(fb, var.first);
            varsOffsets.push_back(fb::CreateNodeVariable(fb, keyOffset, vecOffset));
            flexbuilder.Clear();
        }
        return fb.CreateVectorOfSortedTables(&varsOffsets);
    }
    auto serialize_events(flatbuffers::FlatBufferBuilder& fb, const ThymioNode::EventMap& vars) {
        flexbuffers::Builder flexbuilder;
        std::vector<flatbuffers::Offset<fb::NamedValue>> varsOffsets;
        varsOffsets.reserve(vars.size());
        for(auto&& var : vars.toStdMap()) {
            qfb::to_flexbuffer(var.second, flexbuilder);
            auto& vec = flexbuilder.GetBuffer();
            auto vecOffset = fb.CreateVector(vec);
            auto keyOffset = qfb::add_string(fb, var.first);
            varsOffsets.push_back(fb::CreateNamedValue(fb, keyOffset, vecOffset));
            flexbuilder.Clear();
        }
        return fb.CreateVectorOfSortedTables(&varsOffsets);
    }
}  // namespace detail

Request ThymioDeviceManagerClientEndpoint::setNodeVariables(const ThymioNode& node,
                                                            const ThymioNode::VariableMap& vars) {
    return setVariables(node.uuid(), vars);
}

Request ThymioDeviceManagerClientEndpoint::setGroupVariables(const ThymioGroup& group,
                                                             const ThymioNode::VariableMap& vars) {
    return setVariables(group.uuid(), vars);
}

Request ThymioDeviceManagerClientEndpoint::setVariables(const QUuid& id, const ThymioNode::VariableMap& vars) {
    Request r = prepare_request<Request>();
    flatbuffers::FlatBufferBuilder builder;
    auto uuidOffset = serialize_uuid(builder, id);
    auto varsOffset = detail::serialize_variables(builder, vars);
    write(wrap_fb(builder, fb::CreateSetVariables(builder, r.id(), uuidOffset, varsOffset)));
    return r;
}

Request ThymioDeviceManagerClientEndpoint::setNodeEventsTable(const QUuid& id,
                                                              const QVector<EventDescription>& events) {
    Request r = prepare_request<Request>();
    flatbuffers::FlatBufferBuilder builder;
    auto uuidOffset = serialize_uuid(builder, id);
    std::vector<flatbuffers::Offset<fb::EventDescription>> descOffsets;
    descOffsets.reserve(events.size());
    int i = 0;
    for(auto&& desc : events) {
        auto str_offset = qfb::add_string(builder, desc.name());
        auto descTable = fb::CreateEventDescription(builder, str_offset, desc.size(), i++);
        descOffsets.push_back(descTable);
    }
    write(wrap_fb(builder, fb::CreateRegisterEvents(builder, r.id(), uuidOffset, builder.CreateVector(descOffsets))));
    return r;
}

Request ThymioDeviceManagerClientEndpoint::emitNodeEvents(const ThymioNode& node, const ThymioNode::EventMap& events) {
    Request r = prepare_request<Request>();
    flatbuffers::FlatBufferBuilder builder;
    auto uuidOffset = serialize_uuid(builder, node.uuid());
    auto eventsOffset = detail::serialize_events(builder, events);
    write(wrap_fb(builder, fb::CreateSendEvents(builder, r.id(), uuidOffset, eventsOffset)));
    return r;
}

Request ThymioDeviceManagerClientEndpoint::setScratchPad(const QUuid& id, const QByteArray& data,
                                                         fb::ProgrammingLanguage language) {
    Request r = prepare_request<Request>();
    flatbuffers::FlatBufferBuilder builder;
    auto uuidOffset = serialize_uuid(builder, id);
    auto data_offset = qfb::add_string(builder, data);
    write(wrap_fb(builder,
                  fb::CreateScratchpadUpdate(builder, r.id(), uuidOffset, 0, uuidOffset, language, data_offset)));
    return r;
}

Request ThymioDeviceManagerClientEndpoint::upgradeFirmware(const QUuid& id) {
    Request r = prepare_request<Request>();
    flatbuffers::FlatBufferBuilder builder;
    auto uuidOffset = serialize_uuid(builder, id);
    write(wrap_fb(builder, fb::CreateFirmwareUpgradeRequest(builder, r.id(), uuidOffset)));
    return r;
}

Request ThymioDeviceManagerClientEndpoint::enableWirelessPairingMode() {
    Request r = prepare_request<Request>();
    flatbuffers::FlatBufferBuilder builder;
    write(wrap_fb(builder, fb::CreateEnableThymio2PairingMode(builder)));
    return r;
}

Request ThymioDeviceManagerClientEndpoint::disableWirelessPairingMode() {
    Request r = prepare_request<Request>();
    flatbuffers::FlatBufferBuilder builder;
    write(wrap_fb(builder, fb::CreateEnableThymio2PairingMode(builder, false)));
    return r;
}


Thymio2WirelessDonglePairingRequest ThymioDeviceManagerClientEndpoint::pairThymio2Wireless(const QUuid& dongleId,
                                                                                           const QUuid& nodeId,
                                                                                           quint16 networkId,
                                                                                           quint8 channel) {
    Thymio2WirelessDonglePairingRequest r = prepare_request<Thymio2WirelessDonglePairingRequest>();
    flatbuffers::FlatBufferBuilder builder;
    auto dongleUuidOffset = serialize_uuid(builder, dongleId);
    auto robotUuidOffset = serialize_uuid(builder, nodeId);
    write(wrap_fb(builder,
                  fb::CreateThymio2WirelessDonglePairingRequest(builder, r.id(), dongleUuidOffset, robotUuidOffset,
                                                                networkId, channel)));
    return r;
}

std::vector<std::shared_ptr<ThymioNode>> ThymioDeviceManagerClientEndpoint::nodes(const QUuid& node_or_group_id) const {
    std::vector<std::shared_ptr<ThymioNode>> nodes;
    for(auto&& node : m_nodes) {
        if(node->uuid() == node_or_group_id || node->group_id() == node_or_group_id)
            nodes.push_back(node);
    }
    return nodes;
}

std::shared_ptr<ThymioGroup> ThymioDeviceManagerClientEndpoint::group_from_group_id(const QUuid& id) {
    for(auto&& node : m_nodes) {
        if(node->group_id() == id)
            return node->group();
    }
    return {};
}

std::shared_ptr<ThymioGroup> ThymioDeviceManagerClientEndpoint::group_from_id(const QUuid& id) {
    for(auto&& node : m_nodes) {
        if(node->group_id() == id || node->uuid() == id)
            return node->group();
    }
    return {};
}


QQmlListProperty<ThymioNode> ThymioDeviceManagerClientEndpoint::qml_nodes() {
    static auto at_function = [](QQmlListProperty<ThymioNode>* p, int index) {
        auto that = static_cast<const ThymioDeviceManagerClientEndpoint*>(p->data);
        if(index < 0 || index >= that->m_nodes.size())
            return (ThymioNode*)(nullptr);
        return (that->m_nodes.begin() + index).value().get();
    };
    static auto count_function = [](QQmlListProperty<ThymioNode>* p) {
        return static_cast<const ThymioDeviceManagerClientEndpoint*>(p->data)->m_nodes.size();
    };
    return QQmlListProperty<ThymioNode>(this, this, count_function, at_function);
}


}  // namespace mobsya
