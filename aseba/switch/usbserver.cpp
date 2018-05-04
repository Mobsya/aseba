#include "usbserver.h"
#include "log.h"

namespace mobsya {
usb_server::usb_server(boost::asio::io_context& io_service, std::initializer_list<usb_device_identifier> devices)
    : m_io_ctx(io_service), m_acceptor(io_service, devices) {}

void usb_server::accept() {
    auto session = std::make_shared<usb_connection>(m_io_ctx);
    m_acceptor.accept_async(session->device(), [session, this](boost::system::error_code ec) {
        if(ec) {
            mLogError("system_error: %s", ec.message());
        }
        mLogInfo("Created usb _device");
        usb_device& d = session->device();
        d.open();
        std::string str;
        d.async_read_some(boost::asio::buffer(str), [str = std::move(str)](boost::system::error_code, std::size_t) {

        });
        d.read_some(boost::asio::buffer(str));
        d.write_some(boost::asio::buffer(str));


        // error
        //}
        // d.async_write_some()


        accept();
    });
}

}  // namespace mobsya
