#include "usbserver.h"
#include "log.h"
#include "aseba_endpoint.h"
#include "wireless_configurator_service.h"

namespace mobsya {
usb_server::usb_server(boost::asio::io_context& io_service, std::initializer_list<usb_device_identifier> devices)
    : m_io_ctx(io_service), m_acceptor(io_service, devices) {}


bool usb_server::should_open_for_configuration(const usb_device& d) const {
    auto& service = boost::asio::use_service<wireless_configurator_service>(m_io_ctx);
    return service.is_enabled() && d.usb_device_id().product_id == THYMIO_WIRELESS_DEVICE_ID.product_id;
}

void usb_server::register_configurable_dongle(aseba_device&& d) const {
    auto& service = boost::asio::use_service<wireless_configurator_service>(m_io_ctx);
    service.register_configurable_dongle(std::move(d));
}

void usb_server::accept() {
    auto session = aseba_endpoint::create_for_usb(m_io_ctx);
    m_acceptor.accept_async(session->usb(), [session, this](boost::system::error_code ec) {
        if(ec) {
            mLogError("system_error: %s", ec.message());
        }
        mLogInfo("New Aseba endpoint over USB device connected");
        usb_device& d = session->usb();
        auto res = d.open();
        if(!res) {
            mLogError("Can not open usb device {}", res.error().message());
            accept();
            return;
        }
        if(should_open_for_configuration(d)) {
            register_configurable_dongle(std::move(*session->device()));
        } else {
            d.set_parity(usb_device::parity::none);
            d.set_stop_bits(usb_device::stop_bits::one);
            d.set_data_terminal_ready(true);
            session->set_endpoint_type(aseba_endpoint::endpoint_type::thymio);
            session->set_endpoint_name(d.usb_device_name());
            session->start();
        }
        accept();
    });
}

}  // namespace mobsya
