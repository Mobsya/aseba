#include "usbserver.h"
#include "log.h"
#include "aseba_endpoint.h"

namespace mobsya {
usb_server::usb_server(boost::asio::io_context& io_service, std::initializer_list<usb_device_identifier> devices)
    : m_io_ctx(io_service), m_acceptor(io_service, devices) {}

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
        d.set_baud_rate(usb_device::baud_rate::baud_115200);
        d.set_parity(usb_device::parity::none);
        d.set_stop_bits(usb_device::stop_bits::one);
        d.set_data_terminal_ready(true);
        session->set_endpoint_type(aseba_endpoint::endpoint_type::thymio);
        session->start();
        accept();
    });
}

}  // namespace mobsya
