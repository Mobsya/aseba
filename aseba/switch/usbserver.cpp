#include "usbserver.h"
#include "log.h"

namespace mobsya {
usb_server::usb_server(boost::asio::io_context& io_service, std::initializer_list<usb_device_identifier> devices)
    : m_io_ctx(io_service), m_acceptor(io_service, devices) {}

void usb_server::accept() {
    auto session = std::make_shared<usb_connection>(m_io_ctx);
    m_acceptor.accept_async(session->device(), [session, this](boost::system::error_code ec) {
        if(ec) {
            mobsya::log->error("system_error: %s", ec.message());
        }
        mobsya::log->info("Created usb _device");
        accept();
    });
}

}  // namespace mobsya
