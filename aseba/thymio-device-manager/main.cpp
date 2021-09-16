#include <boost/program_options.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/program_options/cmdline.hpp>
#include <errno.h>
#include "log.h"
#include <fmt/color.h>
#include "interfaces.h"
#include "aseba_node_registery.h"
#include "app_server.h"
#include "app_token_manager.h"
#include "system_sleep_manager.h"
#include "fw_update_service.h"
#include "wireless_configurator_service.h"
#include "aseba_endpoint.h"
#include "aseba_tcpacceptor.h"
#include "uuid_provider.h"
#include <boost/filesystem.hpp>

#ifdef MOBSYA_TDM_ENABLE_USB
#    include "usbserver.h"
#endif
#ifdef MOBSYA_TDM_ENABLE_SERIAL
#    include "serialserver.h"
#endif

static const auto lock_file_path = boost::filesystem::temp_directory_path() / "mobsya-tdm-0accdcbf-eeb2";

struct options {
    bool allow_remote_connections = false;
};

void run_service(boost::asio::io_context& ctx, const options& opts) {

    // Gather a list of local ips so that we can detect connections from
    // the same machine.
    std::map<boost::asio::ip::address, boost::asio::ip::address> local_ips = mobsya::network_interfaces_addresses();
    for(auto&& ip : local_ips) {
        mLogTrace("Local Ip : {} - Mask : {}", ip.first.to_string(), ip.second.to_string());
    }

    [[maybe_unused]] mobsya::uuid_generator& _ = boost::asio::make_service<mobsya::uuid_generator>(ctx);
    mobsya::aseba_node_registery& node_registery = boost::asio::make_service<mobsya::aseba_node_registery>(ctx);
    [[maybe_unused]] mobsya::app_token_manager& token_manager =
        boost::asio::make_service<mobsya::app_token_manager>(ctx);

    [[maybe_unused]] mobsya::system_sleep_manager& system_sleep_manager =
        boost::asio::make_service<mobsya::system_sleep_manager>(ctx);

    // mobsya::firmware_update_service needs to be initialized after mobsya::aseba_node_registery
    [[maybe_unused]] mobsya::firmware_update_service& us =
        boost::asio::make_service<mobsya::firmware_update_service>(ctx);

    [[maybe_unused]] mobsya::wireless_configurator_service& ws =
        boost::asio::make_service<mobsya::wireless_configurator_service>(ctx);
    ws.enable();

    // Create a server for regular tcp connection
    mobsya::application_server<mobsya::tcp::socket> tcp_server(ctx, 8596);
    node_registery.set_tcp_endpoint(tcp_server.endpoint());
   
    mobsya::aseba_tcp_acceptor aseba_tcp_acceptor(ctx);

    // Create a server for websocket
    mobsya::application_server<mobsya::websocket_t> websocket_server(ctx, 8597);
	node_registery.set_ws_endpoint(websocket_server.endpoint());

#ifdef MOBSYA_TDM_ENABLE_USB
    mobsya::usb_server usb_server(ctx, {mobsya::THYMIO2_DEVICE_ID, mobsya::THYMIO_WIRELESS_DEVICE_ID});
    usb_server.accept();
#endif
#ifdef MOBSYA_TDM_ENABLE_SERIAL
    mobsya::serial_server serial_server(ctx, {mobsya::THYMIO2_DEVICE_ID, mobsya::THYMIO_WIRELESS_DEVICE_ID});
    serial_server.accept();
#endif
    aseba_tcp_acceptor.accept();

    websocket_server.allow_remote_connections(opts.allow_remote_connections);
    websocket_server.accept();

    tcp_server.allow_remote_connections(opts.allow_remote_connections);
    tcp_server.accept();

    if(!opts.allow_remote_connections){
        mLogWarn("Remote connections are not allowed");
    }
    mLogTrace("=> TCP Server started on {}", tcp_server.endpoint().port());
    mLogTrace("=> WS Server started on {}", websocket_server.endpoint().port());
    mLogInfo("Server Password: {}", token_manager.password());


    // Enable Bonjour/Zeroconf
    // Make sure this is done after the different servers are started and accept connections
    // So that clients can connect to discovered services
    node_registery.announce_on_zeroconf();

    ctx.run();
}


int start(const options& opts) {
    mLogInfo("Starting...");
    boost::asio::io_context ctx;
    boost::asio::signal_set sig(ctx);
    if(!boost::filesystem::exists(lock_file_path)) {
        std::ofstream _(lock_file_path.c_str());
    }
    boost::interprocess::file_lock lock(lock_file_path.string().c_str());


    // boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work(make_work_guard(ctx));

    if(!lock.try_lock()) {
        mLogError("Thymio Device manager seems to be already running - delete {} if this is not the case",
                  lock_file_path.string());
        return EALREADY;
    }

    boost::filesystem::path token_file =
        boost::filesystem::unique_path(boost::filesystem::temp_directory_path() / "/mobsya-%%%%-%%%%-%%%%-%%%%");

    sig.add(SIGINT);
    sig.add(SIGTERM);
    sig.add(SIGABRT);
    sig.async_wait([&token_file](boost::system::error_code, int sig) {
        mLogWarn("Exiting with signal {}", sig);
        boost::system::error_code ec;
        boost::filesystem::remove(token_file, ec);
        if(ec) {
            mLogWarn("{}", ec.message());
        }
        std::exit(0);
    });

    run_service(ctx, opts);
    return 0;
}

options parse_options(int argc, char** argv) {
    options opts;

    namespace po = boost::program_options;
    po::options_description desc("Options");
    desc.add_options()
        ("help,h", "Show help")
        ("allow-remote-connections", po::bool_switch(&opts.allow_remote_connections),
         "Allow connections from other machines\n(on the local network and internet)")
    ;
    po::variables_map vm;
    try{
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);
    }
    catch(po::error& e) {
        fmt::print(fg(fmt::color::red), "invalid command line arguments: {}\n", e.what());
        exit(1);
    }

    if (vm.count("help")) {
        std::cout << desc << "\n";
        exit(1);
    }
    return opts;
}

int main(int argc, char** argv) {
    auto opts = parse_options(argc, argv);
    try {
        return start(opts);
    } catch(boost::system::system_error& e) {
        mLogError("Exception thrown: {}", e.what());
        std::exit(e.code().value());
    } catch(std::exception& e) {
        mLogError("Exception thrown: {}", e.what());
        std::exit(1);
    } catch(boost::exception&) {
        mLogError("Exception thrown: {}");
        std::exit(1);
    } catch(...) {
        mLogError("Exception thrown: {}");
        std::exit(1);
    }
    return 1;
}
