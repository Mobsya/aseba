#include "aseba_endpoint.h"
#include <aseba/common/utils/utils.h>
#include "aseba_property.h"
#ifdef HAS_FIRMWARE_UPDATE
#   include "fw_update_service.h"
#endif
#include "aseba_node_registery.h"

#ifdef MOBSYA_TDM_ENABLE_SERIAL
#    include "serialacceptor.h"
#endif
#ifdef MOBSYA_TDM_ENABLE_USB
#    include "usbacceptor.h"
#endif
#ifdef HAS_ZEROCONF
#   include "aseba_tcpacceptor.h"
#endif

namespace mobsya {


aseba_endpoint::~aseba_endpoint() {
    destroy();
}

void aseba_endpoint::destroy() {
    if(m_endpoint.is_empty())
        return;
    mLogInfo("Destroying endpoint");
    std::for_each(std::begin(m_nodes), std::end(m_nodes), [](auto&& node) { node.second.node->disconnect(); });
    boost::asio::post([ctx = &m_io_context]() {
        auto& registery = boost::asio::use_service<aseba_node_registery>(*ctx);
        registery.unregister_expired_endpoints();
    });
    stop();
    close();
}

void aseba_endpoint::close() {
    m_endpoint.close();
}
void aseba_endpoint::stop() {
    m_endpoint.stop();
}
void aseba_endpoint::cancel_all_ops() {
    m_endpoint.cancel_all_ops();
}


aseba_endpoint::aseba_endpoint(boost::asio::io_context& io_context, aseba_device&& e, endpoint_type type)
    : m_endpoint(std::move(e))
    , m_strand(io_context.get_executor())
    , m_io_context(io_context)
    , m_endpoint_type(type)
    , m_uuid(boost::asio::use_service<uuid_generator>(io_context).generate()) {}

#ifdef MOBSYA_TDM_ENABLE_SERIAL
aseba_endpoint::pointer aseba_endpoint::create_for_serial(boost::asio::io_context& io) {
    auto ptr = std::shared_ptr<aseba_endpoint>(new aseba_endpoint(io, aseba_device(usb_serial_port(io))));
    ptr->m_group = group::make_group_for_endpoint(io, ptr);
    return ptr;
}
#endif
#ifdef HAS_ZEROCONF
aseba_endpoint::pointer aseba_endpoint::create_for_tcp(boost::asio::io_context& io) {
    auto ptr = std::shared_ptr<aseba_endpoint>(new aseba_endpoint(io, aseba_device(tcp_socket(io))));
    ptr->m_group = group::make_group_for_endpoint(io, ptr);
    return ptr;
}
#endif

void aseba_endpoint::start() {

    auto& registery = boost::asio::use_service<aseba_node_registery>(m_io_context);
    registery.register_endpoint(shared_from_this());


    // A newly connected thymio may not be ready yet
    // Delay asking for its node id to let it start up the vm
    // otherwhise it may never get our request.
    schedule_send_ping(boost::posix_time::milliseconds(200));

    read_aseba_message();

    if(needs_health_check())
        schedule_nodes_health_check();
}

std::optional<std::pair<Aseba::EventDescription, std::size_t>>
aseba_endpoint::get_event(const std::string& name) const {
    auto wname = Aseba::UTF8ToWString(name);
    for(std::size_t i = 0; i < m_defs.events.size(); i++) {
        const auto& event = m_defs.events[i];
        if(event.name == wname) {
            return std::optional<std::pair<Aseba::EventDescription, std::size_t>>(std::pair{event, i});
        }
    }
    return {};
};

std::optional<std::pair<Aseba::EventDescription, std::size_t>> aseba_endpoint::get_event(uint16_t id) const {
    if(m_defs.events.size() <= id)
        return {};
    return std::optional<std::pair<Aseba::EventDescription, std::size_t>>(std::pair{m_defs.events[id], id});
}

boost::system::error_code aseba_endpoint::set_events_table(const group::events_table& events) {
    m_defs.events.clear();
    for(const auto& event : events) {
        if(event.type != event_type::aseba)
            continue;
        m_defs.events.emplace_back(Aseba::UTF8ToWString(event.name), event.size);
    }
    return {};
}

boost::system::error_code aseba_endpoint::set_shared_variables(const variables_map& map) {
    for(auto&& var : map) {
        auto n = Aseba::UTF8ToWString(var.first);
        const auto& p = var.second;
        auto it = std::find_if(m_defs.constants.begin(), m_defs.constants.end(),
                               [n](const Aseba::NamedValue& v) { return n == v.name; });
        if(it != m_defs.constants.end()) {
            m_defs.constants.erase(it);
        }
        if(p.is_null())
            continue;
        if(p.is_integral()) {
            auto v = numeric_cast<int16_t>(property::integral_t(p));
            if(v) {
                m_defs.constants.emplace_back(n, *v);
                continue;
            }
        }
        return make_error_code(error_code::incompatible_variable_type);
    }
    return {};
}

void aseba_endpoint::emit_events(const group::properties_map& map, write_callback&& cb = {}) {
    std::vector<std::shared_ptr<Aseba::Message>> messages;
    messages.reserve(map.size());
    for(auto&& event : map) {
        auto event_def = get_event(event.first);
        if(!event_def) {
            if(cb)
                cb(make_error_code(error_code::no_such_variable));
            return;
        }
        auto bytes = to_aseba_variable(event.second, uint16_t(event_def->first.value));
        if(!bytes) {
            if(cb)
                cb(bytes.error());
            return;
        }
        auto msg = std::make_shared<Aseba::UserMessage>(event_def->second, bytes.value());
        messages.push_back(std::move(msg));
    }
    write_messages(std::move(messages), std::move(cb));
}


void aseba_endpoint::read_aseba_message() {
    auto that = shared_from_this();
    auto cb =
        boost::asio::bind_executor(m_strand, [that](boost::system::error_code ec, std::shared_ptr<Aseba::Message> msg) {
            that->handle_read(ec, msg);
        });

    variant_ns::visit(
        overloaded{[](variant_ns::monostate&) {},
                   [&cb](auto& underlying) { mobsya::async_read_aseba_message(underlying, std::move(cb)); }},
        m_endpoint.ep());
}

void aseba_endpoint::handle_read(boost::system::error_code ec, std::shared_ptr<Aseba::Message> msg) {
    if(ec || !msg) {
        if(!ec && !msg && !m_upgrading_firmware)
            read_aseba_message();

        mLogError("Error while reading aseba message {}", ec ? ec.message() : "Message corrupted");
        return;
    }
    mLogTrace("Message received : '{}' {}", msg->message_name(), ec.message());

    auto node_id = msg->source;
    auto it = m_nodes.find(node_id);
    auto node = it == std::end(m_nodes) ? std::shared_ptr<aseba_node>{} : it->second.node;

    if(msg->type < 0x8000) {
        auto timestamp = std::chrono::system_clock::now();
        auto event = [this, &msg] { return get_event(msg->type); }();
        if(event) {
            on_event(static_cast<const Aseba::UserMessage&>(*msg), event->first, timestamp);
        }
    } else if(!node && (msg->type == ASEBA_MESSAGE_NODE_PRESENT || msg->type == ASEBA_MESSAGE_DESCRIPTION)) {
        const auto protocol_version = (msg->type == ASEBA_MESSAGE_NODE_PRESENT) ?
            static_cast<Aseba::NodePresent*>(msg.get())->version :
            static_cast<Aseba::Description*>(msg.get())->protocolVersion;
        it = m_nodes
                 .insert({node_id,
                          {aseba_node::create(m_io_context, node_id, protocol_version, shared_from_this()),
                           std::chrono::steady_clock::now()}})
                 .first;
        node = it->second.node;
        if(msg->type == ASEBA_MESSAGE_NODE_PRESENT) {
            node->get_description();
            read_aseba_message();
            return;
        }
    }
    if(node) {
        node->on_message(*msg);
        // Update node status
        it->second.last_seen = std::chrono::steady_clock::now();
    }
    if(is_rebooting())
        return;
    read_aseba_message();
}

void aseba_endpoint::remove_node(node_id n) {
    for(auto it = m_nodes.begin(); it != m_nodes.end();) {
        const auto& info = it->second;
        if(info.node && info.node->uuid() == n) {
            info.node->disconnect();
            m_nodes.erase(it);
            return;
        }
    }
}


void aseba_endpoint::on_event(const Aseba::UserMessage& event, const Aseba::EventDescription& def,
                              const std::chrono::system_clock::time_point& timestamp) {
    group::properties_map events;
    auto it = m_nodes.find(event.source);
    if(it == std::end(m_nodes))
        return;

    // create event list
    auto p = detail::aseba_variable_from_range(event.data);
    if((p.is_integral() && def.value != 1) || p.size() != std::size_t(def.value))
        return;
    events.insert(std::pair{Aseba::WStringToUTF8(def.name), p});

    // broadcast to other endpoints
    group()->on_event_received(shared_from_this(), it->second.node->uuid(), events, timestamp);

    // let the node handle the event (send to connected apps)
    it->second.node->on_event_received(events, timestamp);
}


void aseba_endpoint::schedule_send_ping(boost::posix_time::time_duration delay) {
    if(is_rebooting())
        return;

    auto timer = std::make_shared<boost::asio::deadline_timer>(m_io_context);
    timer->expires_from_now(delay);
    std::weak_ptr<aseba_endpoint> ptr = this->shared_from_this();
    auto cb = boost::asio::bind_executor(m_strand, [timer, ptr](const boost::system::error_code& ec) {
        auto that = ptr.lock();
        if(!that || ec)
            return;
        mLogTrace("Requesting list nodes( ec : {} )", ec.message());
        if(that->m_upgrading_firmware)
            return;

        that->write_message(std::make_unique<Aseba::ListNodes>());
        if(that->m_first_ping && that->is_usb()) {
            that->write_message(std::make_unique<Aseba::GetDescription>());
            that->m_first_ping = false;
        }

        if(that->needs_ping())
            that->schedule_send_ping();
    });
    mLogDebug("Waiting before requesting list node");
    timer->async_wait(std::move(cb));
}

void aseba_endpoint::schedule_nodes_health_check(boost::posix_time::time_duration delay) {
    if(m_upgrading_firmware)
        return;

    auto timer = std::make_shared<boost::asio::deadline_timer>(m_io_context);
    timer->expires_from_now(delay);
    std::weak_ptr<aseba_endpoint> ptr = this->shared_from_this();
    auto cb = boost::asio::bind_executor(m_strand, [this, timer, ptr](const boost::system::error_code&) {
        auto that = ptr.lock();
        if(!that)
            return;
        auto now = std::chrono::steady_clock::now();
        for(auto it = m_nodes.begin(); it != m_nodes.end();) {
            const auto& info = it->second;
            auto d = std::chrono::duration_cast<std::chrono::seconds>(now - info.last_seen);
            if(!is_rebooting() && d.count() >= (info.node->get_status() == aseba_node::status::connected ? 25 : 5)) {
                mLogTrace("Node {} has been unresponsive for too long, disconnecting it!",
                          it->second.node->native_id());
                info.node->set_status(aseba_node::status::disconnected);
                it = m_nodes.erase(it);

                if(!is_wireless())
                    this->cancel_all_ops();

            } else {
                ++it;
            }
        }
        // Reschedule
        that->schedule_nodes_health_check();
    });
    timer->async_wait(std::move(cb));
}

#ifdef HAS_FIRMWARE_UPDATE
bool aseba_endpoint::upgrade_firmware(
    uint16_t id, std::function<void(boost::system::error_code ec, double progress, bool complete)> cb,
    firmware_update_options options) {

    if(is_wireless())
        return false;

    m_upgrading_firmware = true;

    auto firmware =
        boost::asio::use_service<firmware_update_service>(m_io_context).firmware_data(aseba_node::node_type::Thymio2);
    firmware.then([id, ptr = shared_from_this(), cb, options](auto f) {
        // if the firmware if empty let it fail as a special case of invalid data
        auto firmware = f.get();
        variant_ns::visit(
            overloaded{
                [](variant_ns::monostate&) {},
#ifdef MOBSYA_TDM_ENABLE_USB
                [&ptr, id, &firmware, &cb, options](usb_device&) {
                    boost::asio::use_service<usb_acceptor_service>(ptr->m_io_context).pause(false);
                    mobsya::upgrade_thymio2_endpoint(
                        ptr->usb().native_handle(), firmware, id,
                        [ptr, cb](boost::system::error_code err, double progress, bool complete) {
                            // Make sure we are running in our executor
                            boost::asio::post(ptr->m_strand, [cb, ptr, err, progress, complete]() {
                                cb(err, progress, complete);
                                // mark the associated device path as unconnected and start accepting devices
                                if(err || complete) {
                                    ptr->device()->free_endpoint();
                                    boost::asio::use_service<usb_acceptor_service>(ptr->m_io_context).pause(false);
                                }
                            });
                        },
                        options);
                },
#endif
#ifdef MOBSYA_TDM_ENABLE_SERIAL
                [&ptr, id, &firmware, &cb, options](usb_serial_port& serial) {
                    // Ignore new device during the update
                    boost::asio::use_service<serial_acceptor_service>(ptr->m_io_context).pause(true);
                    boost::system::error_code ec;
                    serial.close(ec);
                    mobsya::upgrade_thymio2_endpoint(
                        serial.device_path(), firmware, id,
                        [ptr, cb](boost::system::error_code err, double progress, bool complete) {
                            // Make sure we are running in our executor
                            boost::asio::post(ptr->m_strand, [cb, ptr, err, progress, complete]() {
                                cb(err, progress, complete);
                                // mark the associated device path as unconnected and start accepting devices again
                                if(err || complete) {
                                    ptr->device()->free_endpoint();
                                    boost::asio::use_service<serial_acceptor_service>(ptr->m_io_context).pause(false);
                                }
                            });
                        },
                        options);
                },
#endif
                // can never happen
                [](tcp_socket&) {}},

            ptr->m_endpoint.ep());
    });


    return true;
}
#endif

void aseba_endpoint::restore_firmware() {
    if(!is_usb())
        return;
    auto cb = [ptr = shared_from_this()](auto err, auto progress, bool completed) {
        mLogTrace("{} - {} - {}", progress, completed, err);
    };
#ifdef HAS_FIRMWARE_UPDATE
    mLogInfo("Device unresponsive - Attempting to restore firmare");
    upgrade_firmware(0, cb, firmware_update_options::no_reboot);
#endif
}


}  // namespace mobsya
