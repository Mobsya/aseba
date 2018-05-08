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
        mLogInfo("Created usb _device");
        usb_device& d = session->usb();
        d.open();
        session->start();
        accept();
    });
}

}  // namespace mobsya
