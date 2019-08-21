#include "wireless_configurator_service.h"
#include <range/v3/algorithm.hpp>
#include <range/v3/view/map.hpp>
#include <boost/thread.hpp>
#include "aseba_node_registery.h"
#include "serialacceptor.h"

namespace mobsya {


wireless_configurator_service::wireless_configurator_service(boost::asio::execution_context& ctx)
    : boost::asio::detail::service_base<wireless_configurator_service>(static_cast<boost::asio::io_context&>(ctx))
    , m_ctx(static_cast<boost::asio::io_context&>(ctx)) {}

wireless_configurator_service::~wireless_configurator_service() {}

void wireless_configurator_service::enable() {
    m_enabled = true;
    disconnect_all_nodes();

    auto& service = boost::asio::use_service<serial_acceptor_service>(this->m_ctx);
    service.device_unplugged.connect(boost::bind(&wireless_configurator_service::device_unplugged, this, _1));


    // start_node_monitoring(boost::asio::use_service<aseba_node_registery>(m_ctx));
}

void wireless_configurator_service::register_configurable_dongle(usb_serial_port&& d) {
    const std::string id = d.device_path();
    dongle dngle{std::move(d), boost::asio::use_service<uuid_generator>(m_ctx).generate()};
    if(!sync(dngle, false))
        return;
    m_dongles.emplace(std::pair{id, std::move(dngle)});
    if(is_enabled()) {
        wireless_dongles_changed();
        mLogInfo("Wireless Dongle ready for pairing");
    }
}

void wireless_configurator_service::disconnect_all_nodes() {
    auto& service = boost::asio::use_service<aseba_node_registery>(this->m_ctx);
    service.disconnect_all_wireless_endpoints();
}

void wireless_configurator_service::device_unplugged(std::string_view w) {
    auto it = m_dongles.find(std::string(w));
    if(it != m_dongles.end()) {
        m_dongles.erase(it);
        wireless_dongles_changed();
        mLogInfo("Wireless Dongle unplugged");
    }
}


auto wireless_configurator_service::dongles() const -> std::vector<std::reference_wrapper<const dongle>> {
    std::vector<std::reference_wrapper<const dongle>> v;

    for(auto&& e : m_dongles | ranges::view::values) {
        v.emplace_back(e);
    }
    return v;
}

bool wireless_configurator_service::sync(dongle& dongle, bool flash) {
    auto& d = dongle.device;

    boost::system::error_code ec;
    boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
    d.open();
    d.set_option(boost::asio::serial_port::baud_rate(115200), ec);
    d.set_option(boost::asio::serial_port::parity(boost::asio::serial_port::parity::none), ec);
    d.set_option(boost::asio::serial_port::stop_bits(boost::asio::serial_port::stop_bits::one), ec);
    d.set_rts(true);
    d.set_data_terminal_ready(false);

    dongle_data data;
    data.ctrl = flash ? 0x01 : 0;
    if(flash) {
        data.panId = dongle.network_id;
        data.channel = 15 + 5 * dongle.channel;
    }

    if(d.write_some(boost::asio::buffer(&data, sizeof(data)), ec) != sizeof(data))
        return false;
    auto read = d.read_some(boost::asio::buffer(&data, sizeof(data)), ec);

    if(read < sizeof(data) - 1)
        return false;

    dongle.node_id = data.nodeId;
    dongle.network_id = data.panId;
    dongle.channel = (data.channel - uint8_t(15)) / uint8_t(5);

    mLogInfo("node: {}, network{}, channel {}", dongle.node_id, dongle.network_id, dongle.channel);
    d.close();
    return true;
}


auto wireless_configurator_service::configure_dongle(const node_id& n, uint16_t network_id, uint8_t channel)
    -> tl::expected<std::reference_wrapper<const dongle>, configure_error> {

    auto it =
        std::find_if(m_dongles.begin(), m_dongles.end(), [&n](const auto& pair) { return pair.second.uuid == n; });
    if(it == std::end(m_dongles)) {
        return tl::make_unexpected(configure_error::no_such_dongle);
    }
    auto& d = it->second;
    d.network_id = network_id;
    if(channel > 2) {
        return tl::make_unexpected(configure_error::sync_failed);
    }
    d.channel = channel;

    if(!sync(d, true) || d.channel != channel || d.network_id != network_id) {
        m_dongles.erase(it);
        wireless_dongles_changed();
        return tl::make_unexpected(configure_error::sync_failed);
    }

    wireless_dongles_changed();

    return it->second;
}

}  // namespace mobsya
