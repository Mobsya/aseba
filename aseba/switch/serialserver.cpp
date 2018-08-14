#include "serialserver.h"
#include "log.h"
#include "aseba_endpoint.h"

namespace mobsya {
serial_server::serial_server(boost::asio::io_context& io_service, std::initializer_list<usb_device_identifier> devices)
    : m_io_ctx(io_service), m_acceptor(io_service, devices) {}

void serial_server::accept() {
    auto session = aseba_endpoint::create_for_serial(m_io_ctx);
    m_acceptor.accept_async(session->serial(), [session, this](boost::system::error_code ec) {
        if(ec) {
            mLogError("system_error: %s", ec.message());
        }
        mLogInfo("New Aseba endpoint over USB device connected");
        usb_serial_port& d = session->serial();
        d.set_option(boost::asio::serial_port::baud_rate(115200));
        d.set_option(boost::asio::serial_port::parity(boost::asio::serial_port::parity::none));
        d.set_option(boost::asio::serial_port::stop_bits(boost::asio::serial_port::stop_bits::one));
        EscapeCommFunction(d.native_handle(), SETDTR);
        session->start();
        accept();
    });
}

}  // namespace mobsya
