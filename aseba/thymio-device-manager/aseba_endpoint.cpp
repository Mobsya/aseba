#include "aseba_endpoint.h"
#include <aseba/common/utils/utils.h>
#include "aseba_property.h"
#include "thymio2_fwupgrade.h"

namespace mobsya {

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
        auto bytes = to_aseba_variable(event.second, event_def->first.value);
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

bool aseba_endpoint::wireless_enable_configuration_mode(bool enable) {
    if(!is_wireless())
        return false;
    if(!m_wireless_dongle_settings) {
        if(!enable)
            return true;
        m_wireless_dongle_settings = std::make_unique<WirelessDongleSettings>();
    }
    if(m_wireless_dongle_settings->cfg_mode != enable) {
        if(!enable && m_wireless_dongle_settings->pairing) {
            wireless_enable_pairing(false);
        }
#ifdef MOBSYA_TDM_ENABLE_USB
        if(variant_ns::holds_alternative<usb_device>(m_endpoint)) {
            usb().close();
            usb().open();
            usb().set_rts(enable);
            usb().set_data_terminal_ready(!enable);
        }
#endif
#ifdef MOBSYA_TDM_ENABLE_SERIAL
        if(variant_ns::holds_alternative<usb_serial_port>(m_endpoint)) {
            serial().close();
            serial().open();
            serial().set_rts(enable);
            serial().set_data_terminal_ready(!enable);
        }
#endif
    }
    if(enable) {
        if(!sync_wireless_dongle_settings())
            return false;
    }
    m_wireless_dongle_settings->cfg_mode = enable;

    return true;
}

bool aseba_endpoint::sync_wireless_dongle_settings() {
    if(!wireless_cfg_mode_enabled())
        return false;
    const auto s = sizeof(&m_wireless_dongle_settings->data);
    return variant_ns::visit(
        [this, s](auto& device) {
            boost::system::error_code ec;
            if(device.write_some(boost::asio::buffer(&m_wireless_dongle_settings->data, s), ec) != s)
                return false;
            auto read = device.read_some(boost::asio::buffer(&m_wireless_dongle_settings->data, s), ec);
            if(read < s - 1)
                return false;
            return true;
        },
        m_endpoint);
}

bool aseba_endpoint::wireless_enable_pairing(bool enable) {
    if(enable)
        wireless_enable_configuration_mode(true);

    if(m_wireless_dongle_settings->pairing == enable)
        return true;
    m_wireless_dongle_settings->data.ctrl = 0x04;  // toogle pairing
    if(!sync_wireless_dongle_settings())
        return false;
    m_wireless_dongle_settings->pairing = enable;
    return true;
}

bool aseba_endpoint::wireless_set_settings(uint8_t channel, uint16_t networkId, uint16_t dongleId) {
    m_wireless_dongle_settings->data.ctrl = 0;
    m_wireless_dongle_settings->data.channel = 15 + 5 * channel;
    m_wireless_dongle_settings->data.nodeId = dongleId;
    m_wireless_dongle_settings->data.panId = networkId;
    return sync_wireless_dongle_settings();
}

aseba_endpoint::wireless_settings aseba_endpoint::wireless_get_settings() const {
    if(!m_wireless_dongle_settings)
        return {};
    return {m_wireless_dongle_settings->data.panId, m_wireless_dongle_settings->data.nodeId,
            uint8_t(m_wireless_dongle_settings->data.channel - uint8_t(15) / uint8_t(5))};
}

bool aseba_endpoint::wireless_flash() {
    m_wireless_dongle_settings->data.ctrl = 0x04;  // toogle flash
    return sync_wireless_dongle_settings();
}

bool aseba_endpoint::wireless_cfg_mode_enabled() const {
    return m_wireless_dongle_settings && m_wireless_dongle_settings->cfg_mode;
}

bool aseba_endpoint::upgrade_firmware(uint16_t id) {
    if(is_wireless())
        return false;

    /*variant_ns::visit(
        [this, id](auto& underlying) {
            return upgrade_thymio2_endpoint({}, m_io_context, id, underlying,
                                            [ptr = shared_from_this()]() { mLogTrace("Update complete"); });
        },
        m_endpoint);
    */
    return true;
}


}  // namespace mobsya