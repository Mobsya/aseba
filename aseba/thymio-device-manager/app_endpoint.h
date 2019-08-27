#pragma once
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <memory>
#include <type_traits>
#include <queue>
#include "flatbuffers_message_writer.h"
#include "flatbuffers_message_reader.h"
#include "flatbuffers_messages.h"
#include "aseba_node_registery.h"
#include "aseba_nodeid_generator.h"
#include "tdm.h"
#include "log.h"
#include "app_token_manager.h"
#include "utils.h"
#include <pugixml.hpp>

namespace mobsya {
using tcp = boost::asio::ip::tcp;
namespace websocket = boost::beast::websocket;
using websocket_t = websocket::stream<tcp::socket>;

template <typename Self, typename Socket>
class application_endpoint_base : public std::enable_shared_from_this<application_endpoint_base<Self, Socket>> {
public:
    application_endpoint_base(boost::asio::io_context& ctx) = delete;
    template <typename CB>
    void read_message(CB&& handle) = delete;
    void start() = delete;
    void do_write_message(const flatbuffers::DetachedBuffer& buffer) = delete;
    tcp::socket& tcp_socket() = delete;
};

template <typename Self>
class application_endpoint_base<Self, websocket_t>
    : public std::enable_shared_from_this<application_endpoint_base<Self, websocket_t>> {
public:
    application_endpoint_base(boost::asio::io_context& ctx)
        : m_ctx(ctx), m_strand(ctx.get_executor()), m_socket(tcp::socket(ctx)) {}

    template <typename CB>
    void read_message(CB handle) {
        auto that = this->shared_from_this();


        auto cb = boost::asio::bind_executor(
            m_strand,
            [that, handle = std::move(handle)](boost::system::error_code ec, std::size_t bytes_transferred) mutable {
                if(ec) {
                    mLogError("read_message :{}", ec.message());
                    return;
                }
                std::vector<uint8_t> buf(boost::asio::buffers_begin(that->m_buffer.data()),
                                         boost::asio::buffers_begin(that->m_buffer.data()) + bytes_transferred);
                fb_message_ptr msg(std::move(buf));
                handle(ec, std::move(msg));
                that->m_buffer.consume(bytes_transferred);
            });
        m_socket.async_read(m_buffer, std::move(cb));
    }

    void do_write_message(const flatbuffers::DetachedBuffer& buffer) {
        auto that = this->shared_from_this();
        auto cb = boost::asio::bind_executor(m_strand, [that](boost::system::error_code ec, std::size_t s) {
            static_cast<Self&>(*that).handle_write(ec);
        });
        m_socket.async_write(boost::asio::buffer(buffer.data(), buffer.size()), std::move(cb));
    }
    void start() {
        m_socket.binary(true);
        auto that = this->shared_from_this();
        auto cb = boost::asio::bind_executor(
            m_strand, [that](boost::system::error_code ec) mutable { static_cast<Self&>(*that).on_initialized(ec); });
        m_socket.async_accept(std::move(cb));
    }

    tcp::socket& tcp_socket() {
        return m_socket.next_layer();
    }

protected:
    boost::asio::io_context& m_ctx;
    boost::asio::strand<boost::asio::io_context::executor_type> m_strand;

private:
    boost::beast::multi_buffer m_buffer;
    websocket_t m_socket;
};


template <typename Self>
class application_endpoint_base<Self, tcp::socket>
    : public std::enable_shared_from_this<application_endpoint_base<Self, tcp::socket>> {
public:
    application_endpoint_base(boost::asio::io_context& ctx)
        : m_ctx(ctx), m_strand(ctx.get_executor()), m_socket(tcp::socket(ctx)) {}
    template <typename CB>
    void read_message(CB handle) {
        auto that = this->shared_from_this();
        auto cb = boost::asio::bind_executor(
            m_strand, [that, handle = std::move(handle)](boost::system::error_code ec, fb_message_ptr&& msg) {
                handle(ec, std::move(msg));
            });
        mobsya::async_read_flatbuffers_message(m_socket, std::move(cb));
    }

    void do_write_message(const flatbuffers::DetachedBuffer& buffer) {
        auto cb = boost::asio::bind_executor(m_strand, [that = this->shared_from_this()](boost::system::error_code ec) {
            static_cast<Self&>(*that).handle_write(ec);
        });
        mobsya::async_write_flatbuffer_message(m_socket, buffer, std::move(cb));
    }

    void start() {
        static_cast<Self*>(this)->on_initialized();
    }

    tcp::socket& tcp_socket() {
        return m_socket;
    }

protected:
    boost::asio::io_context& m_ctx;
    boost::asio::strand<boost::asio::io_context::executor_type> m_strand;

private:
    tcp::socket m_socket;
};

template <typename Socket>
class application_endpoint : public application_endpoint_base<application_endpoint<Socket>, Socket>,
                             public node_status_monitor,
                             public endpoint_monitor {
public:
    using base = application_endpoint_base<application_endpoint<Socket>, Socket>;
    application_endpoint(boost::asio::io_context& ctx) : base(ctx), m_ctx(ctx), m_pings_timer(ctx) {}

    void set_local(bool is_local) {
        this->m_local_endpoint = is_local;
    }

    void start() {
        mLogInfo("Starting app endpoint");
        base::start();
    }

    void on_initialized(boost::system::error_code ec = {}) {
        mLogTrace("on_initialized: {}", ec.message());

        // start listening for incomming messages
        read_message(
            [this](boost::system::error_code ec, fb_message_ptr&& msg) { this->handle_handshake(ec, std::move(msg)); });

        // Subscribe to node change events
        start_node_monitoring(registery());

        // Subscribe to endpoints change events
        start_endpoints_monitoring(registery());
    }

    template <typename CB>
    void read_message(CB&& handle) {
        base::read_message(std::forward<CB>(handle));
    }

    void read_message() {
        read_message(
            [this](boost::system::error_code ec, fb_message_ptr&& msg) { this->handle_read(ec, std::move(msg)); });
    }

    void write_message(tagged_detached_flatbuffer&& buffer) {
        m_queue.emplace(std::move(buffer));
        if(m_queue.size() > 1 || m_protocol_version == 0)
            return;

        base::do_write_message(m_queue.front().buffer);
    }


    void handle_read(boost::system::error_code ec, fb_message_ptr&& msg) {
        if(ec) {
            mLogError("Network error while reading TDM message {}", ec.message());
            return;
        }
        read_message();  // queue the next read early


        mLogTrace("-> {}", EnumNameAnyMessage(msg.message_type()));
        switch(msg.message_type()) {
            case mobsya::fb::AnyMessage::DeviceManagerShutdownRequest: {
                if(m_local_endpoint) {
#ifdef _WIN32
                    std::quick_exit(0);
#endif
                    m_ctx.stop();
                }
                break;
            }
            case mobsya::fb::AnyMessage::RequestListOfNodes: send_full_node_list(); break;
            case mobsya::fb::AnyMessage::RequestNodeAsebaVMDescription: {
                auto req = msg.as<fb::RequestNodeAsebaVMDescription>();
                send_aseba_vm_description(req->request_id(), req->node_id());
                break;
            }
            case mobsya::fb::AnyMessage::SetVariables: {
                auto vars_msg = msg.as<fb::SetVariables>();
                this->set_variables(vars_msg->request_id(), vars_msg->node_or_group_id(), variables(*vars_msg));
                break;
            }
            case mobsya::fb::AnyMessage::RegisterEvents: {
                auto vars_msg = msg.as<fb::RegisterEvents>();
                this->set_events_table(vars_msg->request_id(), vars_msg->node_or_group_id(),
                                       events_description(*vars_msg));
                break;
            }
            case mobsya::fb::AnyMessage::SendEvents: {
                auto vars_msg = msg.as<fb::SendEvents>();
                this->emit_events(vars_msg->request_id(), vars_msg->node_id(), events(*vars_msg));
                break;
            }
            case mobsya::fb::AnyMessage::RenameNode: {
                auto rename_msg = msg.as<fb::RenameNode>();
                this->rename_node(rename_msg->request_id(), rename_msg->node_id(), rename_msg->new_name()->str());
                break;
            }
            case mobsya::fb::AnyMessage::LockNode: {
                auto lock_msg = msg.as<fb::LockNode>();
                this->lock_node(lock_msg->request_id(), lock_msg->node_id());
                break;
            }
            case mobsya::fb::AnyMessage::UnlockNode: {
                auto lock_msg = msg.as<fb::UnlockNode>();
                this->unlock_node(lock_msg->request_id(), lock_msg->node_id());
                break;
            }
            case mobsya::fb::AnyMessage::CompileAndLoadCodeOnVM: {
                auto req = msg.as<fb::CompileAndLoadCodeOnVM>();
                this->compile_and_send_program(req->request_id(), req->node_id(), vm_language(req->language()),
                                               req->program()->str(), req->options());
                break;
            }
            case mobsya::fb::AnyMessage::SetVMExecutionState: {
                auto req = msg.as<fb::SetVMExecutionState>();
                this->set_vm_execution_state(req->request_id(), req->node_id(), req->command());
                break;
            }
            case mobsya::fb::AnyMessage::WatchNode: {
                auto req = msg.as<fb::WatchNode>();
                this->watch_node_or_group(req->request_id(), req->node_or_group_id(), req->info_type());
                break;
            }
            case mobsya::fb::AnyMessage::SetBreakpoints: {
                auto req = msg.as<fb::SetBreakpoints>();
                this->set_breakpoints(req->request_id(), req->node_id(), breakpoints(*req));
                break;
            }
            case mobsya::fb::AnyMessage::ScratchpadUpdate: {
                auto req = msg.as<fb::ScratchpadUpdate>();
                if(!req->node_id()) {
                    write_message(create_error_response(req->request_id(), fb::ErrorType::unknown_node));
                    break;
                }
                this->update_node_scratchpad(req->request_id(), req->node_id(), req->text()->string_view(),
                                             req->language());
                break;
            }
            case mobsya::fb::AnyMessage::FirmwareUpgradeRequest: {
                auto req = msg.as<fb::FirmwareUpgradeRequest>();
                if(!req->node_id()) {
                    write_message(create_error_response(req->request_id(), fb::ErrorType::unknown_node));
                    break;
                }
                this->upgrade_node_firmware(req->request_id(), req->node_id());
                break;
            }

            case mobsya::fb::AnyMessage::Thymio2WirelessDonglePairingRequest: {
                auto req = msg.as<fb::Thymio2WirelessDonglePairingRequest>();
                if(!req->node_id() || !req->dongle_id()) {
                    write_message(create_error_response(req->request_id(), fb::ErrorType::unknown_node));
                    break;
                }
                this->pair_thymio2_and_dongle(req->request_id(), req->dongle_id(), req->node_id(), req->network_id(),
                                              req->channel_id());
                break;
            }

            default: mLogWarn("Message {} from application unsupported", EnumNameAnyMessage(msg.message_type())); break;
        }
    }

    void handle_write(boost::system::error_code ec) {
        mLogTrace("<- {} : {} ", EnumNameAnyMessage(m_queue.front().tag), ec.message());
        if(ec) {
            mLogError("handle_write : error {}", ec.message());
        }
        m_queue.pop();
        if(!m_queue.empty()) {
            base::do_write_message(m_queue.front().buffer);
        }
    }

    ~application_endpoint() {
        mLogInfo("Stopping app endpoint");

        /* Disconnecting the node monotoring status before unlocking the nodes,
         * otherwise we would receive node status event during destroying the endpoint, leading to a crash */
        node_status_monitor::disconnect();
        endpoint_monitor::disconnect();

        for(auto& p : m_locked_nodes) {
            auto ptr = p.second.lock();
            if(ptr) {
                ptr->unlock(this);
            }
        }
    }

    void node_changed(std::shared_ptr<aseba_node> node, const aseba_node_registery::node_id& id,
                      aseba_node::status status) {
        boost::asio::post(this->m_strand, [that = this->shared_from_this(), node, id, status]() {
            that->do_node_changed(node, id, status);
        });
    }

    void node_variables_changed(std::shared_ptr<aseba_node> node, const variables_map& map,
                                const std::chrono::system_clock::time_point& timestamp) {
        boost::asio::defer(this->m_strand, [that = this->shared_from_this(), node, map, timestamp]() {
            that->do_node_variables_changed(node, map, timestamp);
        });
    }

    void group_variables_changed(std::shared_ptr<group> group, const variables_map& map) {
        boost::asio::defer(this->m_strand, [that = this->shared_from_this(), group, map]() {
            that->do_group_variables_changed(group, map);
        });
    }

    void node_emitted_events(std::shared_ptr<aseba_node> node, const variables_map& events,
                             const std::chrono::system_clock::time_point& timestamp) {
        boost::asio::defer(this->m_strand, [that = this->shared_from_this(), node, events, timestamp]() {
            that->do_node_emitted_events(node, events, timestamp);
        });
    }

    void events_description_changed(std::shared_ptr<group> group, const events_table& events) {
        boost::asio::defer(this->m_strand, [that = this->shared_from_this(), group, events]() {
            that->do_events_description_changed(group, events);
        });
    }

    void scratchpad_changed(std::shared_ptr<group> group, const group::scratchpad& scratchpad) {
        boost::asio::defer(this->m_strand, [that = this->shared_from_this(), group, scratchpad]() {
            that->do_scratchpad_changed(group, scratchpad);
        });
    }

    void node_execution_state_changed(std::shared_ptr<aseba_node> node, const aseba_node::vm_execution_state& state) {
        boost::asio::defer(this->m_strand, [that = this->shared_from_this(), node, state]() {
            that->do_node_execution_state_changed(node, state);
        });
    }

    void endpoints_changed() {
        boost::asio::defer(this->m_strand, [that = this->shared_from_this()]() { that->do_endpoints_changed(); });
    }

private:
    void do_node_changed(std::shared_ptr<aseba_node> node, const aseba_node_registery::node_id& id,
                         aseba_node::status status) {
        // mLogInfo("node changed: {}, {}", node->native_id(), node->status_to_string(status));

        if(status == aseba_node::status::busy && get_locked_node(id)) {
            status = aseba_node::status::ready;
        }
        if(status == aseba_node::status::upgrading)
            status = aseba_node::status::busy;

        flatbuffers::FlatBufferBuilder builder;
        std::vector<flatbuffers::Offset<fb::Node>> nodes;
        flatbuffers::Offset<fb::NodeId> group_id;
        if(auto group = node->group()) {
            group_id = group->uuid().fb(builder);
        }
        nodes.emplace_back(serialize_node(builder, *node, id, status));
        auto vector_offset = builder.CreateVector(nodes);
        auto offset = CreateNodesChanged(builder, vector_offset);
        write_message(wrap_fb(builder, offset));

        if(status == aseba_node::status::disconnected) {
            m_locked_nodes.erase(id);
        }
    }

    void do_node_variables_changed(std::shared_ptr<aseba_node> node, const variables_map& map,
                                   const std::chrono::system_clock::time_point& timestamp) {
        if(!node)
            return;
        write_message(serialize_changed_variables(*node, map, timestamp));
    }

    void do_group_variables_changed(std::shared_ptr<group> grp, const variables_map& map) {
        if(!grp)
            return;
        write_message(serialize_changed_variables(*grp, map));
    }

    void do_node_emitted_events(std::shared_ptr<aseba_node> node, const variables_map& events,
                                const std::chrono::system_clock::time_point& timestamp) {
        if(!node)
            return;
        write_message(serialize_events(*node, events, timestamp));
    }

    void do_events_description_changed(std::shared_ptr<group> group, const events_table& events) {
        write_message(serialize_events_descriptions(*group, events));
    }

    void do_scratchpad_changed(std::shared_ptr<group> group, const group::scratchpad& scratchpad) {
        if(!group)
            return;
        write_message(serialize_scratchpad(*group, scratchpad));
    }

    void do_node_execution_state_changed(std::shared_ptr<aseba_node> node,
                                         const aseba_node::vm_execution_state& state) {
        if(!node)
            return;
        write_message(serialize_execution_state(*node, state));
    }


    void do_endpoints_changed() {
        send_list_of_thymio2_dongles();
    }

    void send_list_of_thymio2_dongles() {
        flatbuffers::FlatBufferBuilder fb;
        auto dongles = registery().thymio2_wireless_dongles();

        std::vector<flatbuffers::Offset<fb::Thymio2WirelessDongle>> offsets;
        std::transform(
            dongles.begin(), dongles.end(), std::back_inserter(offsets), [&fb](std::shared_ptr<aseba_endpoint> ep) {
                auto uuidOffset = ep->uuid().fb(fb);
                const auto settings = ep->wireless_get_settings();
                return mobsya::fb::CreateThymio2WirelessDongle(fb, uuidOffset, settings.network_id, settings.channel);
            });
        auto vecOffset = fb.CreateVector(offsets);
        auto offset = mobsya::fb::CreateThymio2WirelessDonglesChanged(fb, vecOffset);
        write_message(wrap_fb(fb, offset));
    }


    void send_full_node_list() {
        flatbuffers::FlatBufferBuilder builder;
        std::vector<flatbuffers::Offset<fb::Node>> nodes;
        auto map = registery().nodes();
        for(auto& node : map) {
            const auto ptr = node.second.lock();
            if(!ptr)
                continue;
            nodes.emplace_back(serialize_node(builder, *ptr, ptr->uuid(), ptr->get_status()));
        }
        auto vector_offset = builder.CreateVector(nodes);
        auto offset = CreateNodesChanged(builder, vector_offset);
        write_message(wrap_fb(builder, offset));
    }

    auto serialize_node(flatbuffers::FlatBufferBuilder& builder, const aseba_node& n,
                        const aseba_node_registery::node_id& id, aseba_node::status status) {
        if(status == aseba_node::status::busy && get_locked_node(n.uuid())) {
            status = aseba_node::status::ready;
        }
        if(status == aseba_node::status::upgrading)
            status = aseba_node::status::busy;

        auto fw = std::to_string(n.firwmware_version());
        auto afw = std::to_string(n.available_firwmware_version());
        return fb::CreateNodeDirect(builder, id.fb(builder), n.group() ? n.group()->uuid().fb(builder) : 0,
                                    mobsya::fb::NodeStatus(status), n.type(), n.friendly_name().c_str(),
                                    node_capabilities(n), fw.c_str(), afw.c_str());
    }

    uint64_t node_capabilities(const aseba_node& node) const {
        uint64_t caps = 0;
        if(m_local_endpoint) {
            caps |= uint64_t(fb::NodeCapability::ForceResetAndStop);
            if(!node.is_wirelessly_connected())
                caps |= uint64_t(fb::NodeCapability::FirwmareUpgrade);
            if(node.can_be_renamed())
                caps |= uint64_t(fb::NodeCapability::Rename);
        }
        return caps;
    }

    void send_aseba_vm_description(uint32_t request_id, const aseba_node_registery::node_id& id) {
        auto node = registery().node_from_id(id);
        if(!node) {
            // error ?
            return;
        }
        write_message(serialize_aseba_vm_description(request_id, *node, id));
    }

    void rename_node(uint32_t request_id, const aseba_node_registery::node_id& id, const std::string& new_name) {
        auto n = registery().node_from_id(id);
        if(!n || !(node_capabilities(*n) & uint64_t(fb::NodeCapability::Rename))) {
            mLogWarn("rename_node: node {} does not exist or can not be renamed", id);
            write_message(create_error_response(request_id, fb::ErrorType::unknown_node));
            return;
        }
        n->rename(new_name);
        write_message(create_ack_response(request_id));
    }

    void lock_node(uint32_t request_id, const aseba_node_registery::node_id& id) {
        auto node = registery().node_from_id(id);
        if(!node) {
            write_message(create_error_response(request_id, fb::ErrorType::unknown_node));
            return;
        }
        m_locked_nodes[id] = node;
        bool res = node->lock(this);
        if(!res) {
            m_locked_nodes.erase(id);
            write_message(create_error_response(request_id, fb::ErrorType::node_busy));
        } else {
            write_message(create_ack_response(request_id));
        }
    }

    void unlock_node(uint32_t request_id, const aseba_node_registery::node_id& id) {
        auto it = m_locked_nodes.find(id);
        std::shared_ptr<aseba_node> node;
        if(it != std::end(m_locked_nodes)) {
            node = it->second.lock();
            m_locked_nodes.erase(it);
        }

        if(!node) {
            write_message(create_error_response(request_id, fb::ErrorType::unknown_node));
            return;
        }
        if(!node->unlock(this)) {
            write_message(create_error_response(request_id, fb::ErrorType::node_busy));
        } else {
            write_message(create_ack_response(request_id));
        }
    }

    void set_variables(uint32_t request_id, const aseba_node_registery::node_id& id, variables_map m) {
        auto n = get_locked_node(id);
        if(n) {
            set_node_variables(request_id, id, m);
        } else {
            set_group_variables(request_id, id, m);
        }
    }

    void set_group_variables(uint32_t request_id, const aseba_node_registery::node_id& id, variables_map m) {
        auto grp = get_group(id);
        if(!grp) {
            mLogWarn("set_group_variables: no such group", id);
            write_message(create_error_response(request_id, fb::ErrorType::unknown_node));
            return;
        }
        auto err = grp->set_shared_variables(m);
        if(err) {
            mLogWarn("set_group_variables: invalid variables", id);
            write_message(create_error_response(request_id, fb::ErrorType::unsupported_variable_type));
            return;
        }
        write_message(create_ack_response(request_id));
    }

    void set_node_variables(uint32_t request_id, const aseba_node_registery::node_id& id, variables_map m) {
        auto n = get_locked_node(id);
        if(!n) {
            mLogWarn("set_node_variables: node {} not locked", id);
            write_message(create_error_response(request_id, fb::ErrorType::unknown_node));
            return;
        }
        auto err = n->set_node_variables(m, create_device_write_completion_cb(request_id));
        if(err) {
            mLogWarn("set_node_variables: invalid variables", id);
            write_message(create_error_response(request_id, fb::ErrorType::unsupported_variable_type));
        }
    }

    void set_events_table(uint32_t request_id, const aseba_node_registery::node_id& id, events_table events) {
        auto g = get_group(id);
        if(!g) {
            mLogWarn("set_events_table: {} is not a group associated to a locked node", id);
            write_message(create_error_response(request_id, fb::ErrorType::unknown_node));
            return;
        }
        auto err = g->set_events_table(events);
        if(err) {
            mLogWarn("set_node_events_table: invalid events", id);
            write_message(create_error_response(request_id, fb::ErrorType::unsupported_variable_type));
        } else {
            write_message(create_ack_response(request_id));
        }
    }

    void emit_events(uint32_t request_id, const aseba_node_registery::node_id& id, group::properties_map m) {
        auto g = get_group(id);
        if(!g) {
            mLogWarn("emits_events: {} is not a group associated to a locked node", id);
            write_message(create_error_response(request_id, fb::ErrorType::unknown_node));
            return;
        }
        g->emit_events(m, create_device_write_completion_cb(request_id));
    }

    void compile_and_send_program(uint32_t request_id, const aseba_node_registery::node_id& id, vm_language language,
                                  std::string program, fb::CompilationOptions opts) {
        auto n = get_locked_node(id);
        if(!n) {
            auto g = get_group(id);
            if(g) {
                auto ec = g->load_code(program, language);
                if(ec) {
                    write_message(create_error_response(request_id, fb::ErrorType::unknown_error));
                    return;
                }
                for(auto&& s : g->scratchpads()) {
                    do_scratchpad_changed(g, s);
                }

                write_message(create_ack_response(request_id));
                return;
            }
        }
        if(!n) {
            mLogWarn("send_aseba_code: node {} not locked", id);
            write_message(create_error_response(request_id, fb::ErrorType::unknown_node));
            return;
        }
        auto callback = [request_id, strand = this->m_strand,
                         ptr = weak_from_this()](boost::system::error_code ec, aseba_node::compilation_result result) {
            boost::asio::post(strand, [ec, result, request_id, ptr]() {
                auto that = ptr.lock();
                if(!that)
                    return;
                if(ec) {
                    that->write_message(create_error_response(request_id, fb::ErrorType::unknown_node));
                    return;
                }
                that->write_message(create_compilation_result_response(request_id, result));
            });
        };
        if((int32_t(opts) & int32_t(fb::CompilationOptions::LoadOnTarget))) {
            n->compile_and_send_program(language, program, callback);
        } else {
            n->compile_program(language, program, callback);
        }
    }

    void set_vm_execution_state(uint32_t request_id, aseba_node_registery::node_id id,
                                fb::VMExecutionStateCommand cmd) {
        auto n = get_locked_node(id);
        if(!n && cmd == fb::VMExecutionStateCommand::Stop) {
            n = registery().node_from_id(id);
            if(n && !(node_capabilities(*n) & uint64_t(fb::NodeCapability::ForceResetAndStop)))
                n = {};
        }
        if(!n) {
            mLogWarn("set_vm_execution_state: node {} not locked", id);
            write_message(create_error_response(request_id, fb::ErrorType::unknown_node));
            return;
        }
        n->set_vm_execution_state(cmd, create_device_write_completion_cb(request_id));
    }

    void set_breakpoints(uint32_t request_id, aseba_node_registery::node_id id, std::vector<breakpoint> breakpoints) {
        auto n = get_locked_node(id);
        if(!n) {
            mLogWarn("set_breakpoints: node {} not locked", id);
            write_message(create_error_response(request_id, fb::ErrorType::unknown_node));
            return;
        }
        auto callback = [request_id, strand = this->m_strand, ptr = weak_from_this()](boost::system::error_code ec,
                                                                                      aseba_node::breakpoints bps) {
            boost::asio::post(strand, [ec, bps, request_id, ptr]() {
                auto that = ptr.lock();
                if(!that)
                    return;
                that->write_message(create_set_breakpoint_response(
                    request_id, ec ? fb::ErrorType::unknown_error : fb::ErrorType::no_error, bps));
            });
        };

        n->set_breakpoints(breakpoints, callback);
    }

    void update_node_scratchpad(uint32_t request_id, node_id id, std::string_view content,
                                fb::ProgrammingLanguage language) {
        const auto n = get_locked_node(id);
        std::shared_ptr<group> g;
        if(n) {
            g = n->group();
        }
        if(!g) {
            mLogWarn("update_node_scratchpad: node {} not locked", id);
            write_message(create_error_response(request_id, fb::ErrorType::unknown_node));
            return;
        }
        g->set_node_scratchpad(id, content, language);
        write_message(create_ack_response(request_id));
    }

    void upgrade_node_firmware(uint32_t request_id, node_id id) {
        const std::shared_ptr<aseba_node> n = registery().node_from_id(id);
        if(!n || n->get_status() != aseba_node::status::available ||
           !(node_capabilities(*n) & uint64_t(fb::NodeCapability::FirwmareUpgrade))) {
            mLogWarn("upgrade_node_firmware: node {} does not exist, is locked or can not be upgraded", id);
            write_message(create_error_response(request_id, fb::ErrorType::unknown_node));
            return;
        }
        auto update_started = n->upgrade_firmware(
            [ptr = shared_from_this(), request_id, id](boost::system::error_code err, double progresss, bool complete) {
                if(err) {
                    ptr->write_message(create_error_response(request_id, fb::ErrorType::unknown_error));
                    return;
                } else if(complete) {
                    ptr->write_message(create_ack_response(request_id));
                    return;
                } else {
                    flatbuffers::FlatBufferBuilder builder;
                    const auto node_offset = id.fb(builder);
                    ptr->write_message(
                        wrap_fb(builder, fb::CreateFirmwareUpgradeStatus(builder, request_id, node_offset, progresss)));
                }
            });

        if(!update_started) {
            write_message(create_error_response(request_id, fb::ErrorType::unknown_error));
        }
    }

    void pair_thymio2_and_dongle(uint32_t request_id, node_id dongle_id, node_id robot_id, uint16_t network_id,
                                 uint8_t channel) {
        auto dongle = registery().thymio2_wireless_dongle(dongle_id);
        auto node = registery().node_from_id(robot_id);
        if(!dongle || !node) {
            // error
            return;
        }
        auto d = boost::asio::use_service<aseba_nodeid_generator>(m_ctx).generate();

        aseba_endpoint::wireless_settings settings = dongle->wireless_get_settings();

        mLogTrace("Current wireless settings: node id   = {}, network id = {}", settings.dongle_id,
                  settings.network_id);

        if(settings.network_id != network_id) {
            if(!dongle->wireless_set_settings(network_id, channel)) {
                // error
                return;
                settings = dongle->wireless_get_settings();
                mLogTrace("new wireless settings: node id   = {}, network id = {}", settings.dongle_id,
                          settings.network_id);
            }
        }


        auto n = boost::asio::use_service<aseba_nodeid_generator>(m_ctx).generate();

        settings = dongle->wireless_get_settings();
        if(!node->set_rf_settings(settings.network_id, node->native_id(), settings.channel)) {
            // error
            return;
        }
        mLogTrace("New wireless setting for node: node id   = {}, network id = {}, channel = {}", node->native_id(),
                  settings.network_id, channel);
    }
    void watch_node_or_group(uint32_t request_id, const aseba_node_registery::node_id& id, uint32_t flags) {
        auto group = registery().group_from_id(id);
        auto node = registery().node_from_id(id);
        if(!node && !group) {
            write_message(create_error_response(request_id, fb::ErrorType::unknown_node));
            return;
        }
        if(group &&
           ((flags & uint32_t(fb::WatchableInfo::SharedVariables)) ||
            (flags & uint32_t(fb::WatchableInfo::SharedEventsDescription)) ||
            (flags & uint32_t(fb::WatchableInfo::Scratchpads)))) {

            if(flags & uint32_t(fb::WatchableInfo::SharedVariables)) {
                if(!m_watch_nodes[fb::WatchableInfo::SharedVariables].count(id)) {
                    auto variables = group->shared_variables();
                    this->group_variables_changed(group, variables);
                    m_watch_nodes[fb::WatchableInfo::SharedVariables][id] = group->connect_to_variables_changes(
                        std::bind(&application_endpoint::group_variables_changed, this, std::placeholders::_1,
                                  std::placeholders::_2));
                } else if(group->uuid() == id) {
                    m_watch_nodes[fb::WatchableInfo::SharedVariables].erase(id);
                }
            }

            if(flags & uint32_t(fb::WatchableInfo::SharedEventsDescription)) {
                if(!m_watch_nodes[fb::WatchableInfo::SharedEventsDescription].count(id)) {
                    auto events = group->get_events_table();
                    this->events_description_changed(group, events);

                    m_watch_nodes[fb::WatchableInfo::SharedEventsDescription][id] =
                        group->connect_to_events_description_changes(
                            std::bind(&application_endpoint::events_description_changed, this, std::placeholders::_1,
                                      std::placeholders::_2));
                } else if(group->uuid() == id) {
                    m_watch_nodes[fb::WatchableInfo::SharedEventsDescription].erase(id);
                }
            }

            if(flags & uint32_t(fb::WatchableInfo::Scratchpads)) {
                if(!m_watch_nodes[fb::WatchableInfo::Scratchpads].count(id)) {
                    m_watch_nodes[fb::WatchableInfo::Scratchpads][id] = group->connect_to_scratchpad_updates(std::bind(
                        &application_endpoint::scratchpad_changed, this, std::placeholders::_1, std::placeholders::_2));

                    for(auto&& s : group->scratchpads()) {
                        this->scratchpad_changed(group, s);
                    }


                } else if(group->uuid() == id) {
                    m_watch_nodes[fb::WatchableInfo::Scratchpads].erase(id);
                }
            }
        }
        if(node) {
            if(flags & uint32_t(fb::WatchableInfo::Variables)) {
                if(!m_watch_nodes[fb::WatchableInfo::Variables].count(id)) {
                    auto variables = node->variables();
                    this->node_variables_changed(node, variables, std::chrono::system_clock::now());
                }
                m_watch_nodes[fb::WatchableInfo::Variables][id] = node->connect_to_variables_changes(
                    std::bind(&application_endpoint::node_variables_changed, this, std::placeholders::_1,
                              std::placeholders::_2, std::placeholders::_3));
            } else {
                m_watch_nodes[fb::WatchableInfo::Variables].erase(id);
            }

            if(flags & uint32_t(fb::WatchableInfo::Events)) {
                m_watch_nodes[fb::WatchableInfo::Events][id] = node->connect_to_events(
                    std::bind(&application_endpoint::node_emitted_events, this, std::placeholders::_1,
                              std::placeholders::_2, std::placeholders::_3));
            } else {
                m_watch_nodes[fb::WatchableInfo::Events].erase(id);
            }

            if(flags & uint32_t(fb::WatchableInfo::VMExecutionState)) {
                m_watch_nodes[fb::WatchableInfo::VMExecutionState][id] = node->connect_to_execution_state_changes(
                    std::bind(&application_endpoint::node_execution_state_changed, this, std::placeholders::_1,
                              std::placeholders::_2));
                this->node_execution_state_changed(node, node->execution_state());

            } else {
                m_watch_nodes[fb::WatchableInfo::VMExecutionState].erase(id);
            }
        }

        write_message(create_ack_response(request_id));
    }

    aseba_node_registery& registery() {
        return boost::asio::use_service<aseba_node_registery>(this->m_ctx);
    }

    std::shared_ptr<aseba_node> get_locked_node(const aseba_node_registery::node_id& id) const {
        auto it = m_locked_nodes.find(id);
        if(it == std::end(m_locked_nodes))
            return {};
        return it->second.lock();
    }

    std::shared_ptr<group> get_group(const aseba_node_registery::node_id& id) const {
        auto it = std::find_if(std::begin(m_locked_nodes), std::end(m_locked_nodes), [&id](const auto& pair) {
            if(id == pair.first)
                return true;
            auto locked = pair.second.lock();
            return locked && locked->group() && locked->group()->uuid() == id;
        });
        if(it == std::end(m_locked_nodes))
            return {};
        auto locked = it->second.lock();
        if(!locked)
            return {};
        return locked->group();
    }

    /*
     *  Returns a std::function that, when called posts a lambda in the endpoint strand
     *  Said lambda is ultimately responsible for sending the ack message to the app,
     *  if it still exists.
     */
    aseba_node::write_callback create_device_write_completion_cb(uint32_t request_id) {
        auto strand = this->m_strand;
        auto ptr = weak_from_this();
        auto callback = [request_id, strand, ptr](boost::system::error_code ec) {
            boost::asio::post(strand, [ec, request_id, ptr]() {
                auto that = ptr.lock();
                if(!that)
                    return;
                if(!ec) {
                    that->write_message(create_ack_response(request_id));
                } else {
                    that->write_message(create_error_response(request_id, fb::ErrorType::node_busy));
                }
            });
        };
        return callback;
    }

    void handle_handshake(boost::system::error_code ec, fb_message_ptr&& msg) {
        if(ec) {
            mLogError("Network error while reading TDM message {}", ec.message());
            return;
        }

        if(msg.message_type() != mobsya::fb::AnyMessage::ConnectionHandshake) {
            mLogError("Client did not send a ConnectionHandshake message");
            return;
        }
        auto hs = msg.as<fb::ConnectionHandshake>();
        if(hs->protocolVersion() < tdm::minProtocolVersion || tdm::protocolVersion < hs->minProtocolVersion()) {
            mLogError("Client protocol version ({}) is not compatible with this server({}+)", hs->protocolVersion(),
                      tdm::minProtocolVersion);
        } else {
            m_protocol_version = std::min(hs->protocolVersion(), tdm::protocolVersion);
            m_max_out_going_packet_size = hs->maxMessageSize();
            auto& token_manager = boost::asio::use_service<app_token_manager>(m_ctx);
            // TODO ?
            if(hs->token())
                token_manager.check_token(app_token_manager::token_view{hs->token()->data(), hs->token()->size()});
        }
        flatbuffers::FlatBufferBuilder builder;
        write_message(wrap_fb(builder,
                              fb::CreateConnectionHandshake(builder, tdm::minProtocolVersion, m_protocol_version,
                                                            tdm::maxAppEndPointMessageSize, 0, m_local_endpoint)));

        // the client do not have a compatible protocol version, bailing out
        if(m_protocol_version == 0) {
            return;
        }

        // Once the handshake is complete, send a list of nodes, that will also flush out all pending outgoing
        // messages
        send_full_node_list();
        send_list_of_thymio2_dongles();

        start_sending_pings();
        read_message();
    }

    void start_sending_pings() {
        m_pings_timer.expires_from_now(boost::posix_time::milliseconds(2500));
        m_pings_timer.async_wait([ptr = weak_from_this()](boost::system::error_code ec) {
            if(ec)
                return;
            if(auto that = ptr.lock()) {
                flatbuffers::FlatBufferBuilder builder;
                that->write_message(wrap_fb(builder, fb::CreatePing(builder)));
                that->start_sending_pings();
            }
        });
    }
    boost::asio::deadline_timer m_pings_timer;

    std::shared_ptr<application_endpoint<Socket>> shared_from_this() {
        return std::static_pointer_cast<application_endpoint<Socket>>(base::shared_from_this());
    }

    std::weak_ptr<application_endpoint<Socket>> weak_from_this() {
        return std::static_pointer_cast<application_endpoint<Socket>>(this->shared_from_this());
    }

    boost::asio::io_context& m_ctx;
    std::queue<tagged_detached_flatbuffer> m_queue;
    std::unordered_map<aseba_node_registery::node_id, std::weak_ptr<aseba_node>, boost::hash<boost::uuids::uuid>>
        m_locked_nodes;
    std::unordered_map<fb::WatchableInfo,
                       std::unordered_map<aseba_node_registery::node_id, boost::signals2::scoped_connection>>
        m_watch_nodes;
    uint16_t m_protocol_version = 0;
    uint16_t m_max_out_going_packet_size = 0;
    bool m_local_endpoint = false;
};  // namespace mobsya

}  // namespace mobsya
