#include "serialacceptor.h"
#include "aseba_endpoint.h"
#include "thymio2_fwupgrade.h"

namespace mobsya {
class thymio2_upgrade_service {
public:
    thymio2_upgrade_service(boost::asio::io_context& io_ctx, thymio2_firmware_data firmware)
        : m_io_ctx(io_ctx), m_acceptor(io_ctx, {mobsya::THYMIO2_DEVICE_ID}), m_firmware(firmware) {}

    void accept() {
        auto port = std::make_shared<usb_serial_port>(m_io_ctx);
        m_acceptor.accept_async(*port, [port, this](boost::system::error_code ec) {
            if(ec) {
                return accept();
            }

            port->close();
            if(ec) {
                mLogError("system_error: %s", ec.message());
            }
            mLogInfo("New Aseba endpoint over USB device connected");
            mobsya::details::upgrade_thymio2_endpoint("/dev/ttyACM0", m_firmware, 0,
                                                      [](auto err, auto progress, bool completed) {
                                                          mLogTrace("{} - {} - {}", progress, completed, err);
                                                          if(err)
                                                              exit(1);
                                                          if(completed)
                                                              exit(0);
                                                      });
        });
    }

private:
    boost::asio::io_context& m_io_ctx;
    serial_acceptor m_acceptor;
    thymio2_firmware_data m_firmware;
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

    mobsya::thymio2_firmware_data data(pages.value().begin(), pages.value().end());
    boost::asio::io_context ctx;
    mobsya::thymio2_upgrade_service service(ctx, data);
    service.accept();
    ctx.run();
}