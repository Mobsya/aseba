#ifndef NDEBUG
#    pragma comment(linker, "/SUBSYSTEM:CONSOLE")
#endif

#include "serialacceptor.h"
#include "aseba_endpoint.h"
#include "thymio2_fwupgrade.h"
#include <fstream>
#include <boost/program_options.hpp>

namespace mobsya {
class thymio2_upgrade_service {
public:
    thymio2_upgrade_service(boost::asio::io_context& io_ctx, thymio2_firmware_data firmware,
                            firmware_update_options opts)
        : m_io_ctx(io_ctx)
        ,
#ifdef MOBSYA_TDM_ENABLE_SERIAL
        m_acceptor(io_ctx, {mobsya::THYMIO2_DEVICE_ID})
        ,
#endif
        m_firmware(firmware)
        , m_opts(opts) {
    }

    void accept() {
#ifdef MOBSYA_TDM_ENABLE_SERIAL
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
            mobsya::details::upgrade_thymio2_serial_endpoint(port->device_path(), m_firmware, 0,
                                                             [](auto err, auto progress, bool completed) {
                                                                 mLogTrace("{} - {} - {}", progress, completed, err);
                                                                 if(err)
                                                                     exit(1);
                                                                 if(completed)
                                                                     exit(0);
                                                             },
                                                             m_opts);
        });
#endif
    }

private:
    boost::asio::io_context& m_io_ctx;
#ifdef MOBSYA_TDM_ENABLE_SERIAL
    serial_acceptor m_acceptor;
#endif
    thymio2_firmware_data m_firmware;
    firmware_update_options m_opts;
};


}  // namespace mobsya

void help(const char* name, const boost::program_options::options_description& desc) {
    std::cout << "Usage: " << name << " [options] <firmware-path>\n";
    std::cout << desc;
}

int main(int argc, char** argv) {

    namespace po = boost::program_options;

    po::options_description desc{"Install a firmware on a thymio 2"};
    desc.add_options()("help,h", "Help")("firmware-hex-file", po::value<std::string>(),
                                         "Path of the firmware file(.hex)")(
        "no-reboot", po::bool_switch(),
        "Do not attempt to reboot the device - this is useful when the firmware has been corrupted");

    po::positional_options_description positional_desc;
    positional_desc.add("firmware-hex-file", 1);

    po::variables_map vm;
    try {
        po::store(po::command_line_parser(argc, argv).options(desc).positional(positional_desc).run(), vm);
    } catch(po::error& e) {
        std::cout << e.what();
        return 1;
    }

    if(vm.count("help")) {
        help(argv[0], desc);
        return 0;
    }
    po::notify(vm);

    if(vm.count("firmware-hex-file") != 1) {
        help(argv[0], desc);
        return 0;
    }

    auto opts = mobsya::firmware_update_options::no_option;
    if(vm["no-reboot"].as<bool>())
        opts = mobsya::firmware_update_options::no_reboot;

    const auto file_name = vm["firmware-hex-file"].as<std::string>();
    std::ifstream file(file_name);
    if(file.fail()) {
        mLogError("Unable to open firmware file {}", file_name);
        return 1;
    }
    std::string firmware((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    auto pages = mobsya::details::extract_pages_from_hex(firmware);
    if(!pages.has_value()) {
        mLogError("{} is not a valid firmware or is corrupted", file_name);
        return 1;
    }

    mobsya::thymio2_firmware_data data(pages.value().begin(), pages.value().end());
    boost::asio::io_context ctx;
    mobsya::thymio2_upgrade_service service(ctx, data, opts);
    service.accept();
    ctx.run();
}
