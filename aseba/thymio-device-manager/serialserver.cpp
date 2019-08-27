#include "serialserver.h"
#include "log.h"
#include "aseba_endpoint.h"
#include "wireless_configurator_service.h"

namespace mobsya {
serial_server::serial_server(boost::asio::io_context& io_service, std::initializer_list<usb_device_identifier> devices)
    : m_io_ctx(io_service), m_acceptor(io_service, devices) {}


void create_endpoint(aseba_endpoint::pointer session, usb_serial_port& d) {
    boost::system::error_code ec;
    d.open();
    d.set_option(boost::asio::serial_port::baud_rate(115200), ec);
    d.set_option(boost::asio::serial_port::parity(boost::asio::serial_port::parity::none), ec);
    d.set_option(boost::asio::serial_port::stop_bits(boost::asio::serial_port::stop_bits::one), ec);
    d.set_data_terminal_ready(true);
    session->set_endpoint_type(aseba_endpoint::endpoint_type::thymio);
    session->set_endpoint_name(d.device_name());
    session->start();
}

bool serial_server::should_open_for_configuration(const usb_serial_port& d) const {
    auto& service = boost::asio::use_service<wireless_configurator_service>(m_io_ctx);
    return service.is_enabled() && d.usb_device_id().product_id == THYMIO_WIRELESS_DEVICE_ID.product_id;
}

void serial_server::register_configurable_dongle(usb_serial_port&& d) const {
    auto& service = boost::asio::use_service<wireless_configurator_service>(m_io_ctx);
    aseba_device ad(std::move(d));
    service.register_configurable_dongle(std::move(ad));
}


void serial_server::accept() {
    auto session = aseba_endpoint::create_for_serial(m_io_ctx);
    m_acceptor.accept_async(session->serial(), [session, this](boost::system::error_code ec) {
        if(ec) {
            mLogError("system_error: %s", ec.message());
        }
        mLogInfo("New Aseba endpoint over USB device connected");
        usb_serial_port& d = session->serial();
        if(should_open_for_configuration(d)) {
            register_configurable_dongle(std::move(d));
        } else {
            create_endpoint(session, d);
        }
        accept();
    });
}


}  // namespace mobsya
