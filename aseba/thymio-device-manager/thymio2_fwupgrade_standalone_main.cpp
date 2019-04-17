#include "serialacceptor.h"
#include "aseba_endpoint.h"
#include "thymio2_fwupgrade.h"

namespace mobsya {
class thymio2_upgrade_service {
public:
    thymio2_upgrade_service(boost::asio::io_context& io_ctx, firmware_data firmware)
        : m_io_ctx(io_ctx), m_acceptor(io_ctx, {mobsya::THYMIO2_DEVICE_ID}), m_firmware(firmware) {}

    void accept() {
        auto port = std::make_shared<usb_serial_port>(m_io_ctx);
        m_acceptor.accept_async(*port, [port, this](boost::system::error_code ec) {
            if(ec) {
                return accept();
            }

            port->set_option(boost::asio::serial_port::baud_rate(115200));
            port->set_option(boost::asio::serial_port::parity(boost::asio::serial_port::parity::none));
            port->set_option(boost::asio::serial_port::stop_bits(boost::asio::serial_port::stop_bits::one));
            port->set_data_terminal_ready(true);
            if(ec) {
                mLogError("system_error: %s", ec.message());
            }
            mLogInfo("New Aseba endpoint over USB device connected");
            /*upgrade_thymio2_endpoint(m_firmware, m_io_ctx, 0, *port, [this, port]() {
                mLogInfo("Upgrade complete");
                // accept();
            });*/
        });
    }

private:
    boost::asio::io_context& m_io_ctx;
    serial_acceptor m_acceptor;
    firmware_data m_firmware;
};


}  // namespace mobsya

int main(int argc, char** argv) {

    std::ifstream file(argv[1]);
    std::string firmware((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    auto pages = mobsya::details::extract_pages_from_hex(firmware);
    if(!pages.has_value()) {
        // error
        return 1;
    }

    mobsya::firmware_data data(pages.value().begin(), pages.value().end());
    // The page with 0 index shall be written last - for some reason?
    // auto it = std::find_if(data.begin(), data.end(), [](auto&& pair) { return pair.first == 0; });
    // std::iter_swap(it, data.end() - 1);

    // boost::asio::io_context ctx;
    // mobsya::thymio2_upgrade_service service(ctx, data);
    mobsya::upgrade_thymio2_endpoint(data, 55301);


    // service.accept();
    // ctx.run();
}