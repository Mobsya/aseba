#include <boost/program_options.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/interprocess/sync/file_lock.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <errno.h>
#include "log.h"
#include "interfaces.h"
#include "aseba_node_registery.h"
#include "app_server.h"
#include "aseba_endpoint.h"
#include "aseba_tcpacceptor.h"

#ifdef MOBSYA_TDM_ENABLE_USB
#    include "usbserver.h"
#endif
#ifdef MOBSYA_TDM_ENABLE_SERIAL
#    include "serialserver.h"
#endif

static const auto lock_file_path = boost::filesystem::temp_directory_path() / "mobsya-tdm-0accdcbf-eeb2";

int main() {
    mLogInfo("Starting...");
    boost::asio::io_context ctx;
    boost::asio::signal_set sig(ctx);
    if(!boost::filesystem::exists(lock_file_path)) {
        std::ofstream _(lock_file_path.c_str());
    }
    boost::interprocess::file_lock lock(lock_file_path.string().c_str());

    try {

        boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work(make_work_guard(ctx));

        if(!lock.try_lock()) {
            mLogError("Thymio Device manager seems to be already running - delete {} if this is not the case",
                      lock_file_path.string());
            return EALREADY;
        }

        sig.add(SIGINT);
        sig.add(SIGTERM);
        sig.add(SIGABRT);
        sig.async_wait([](boost::system::error_code, int sig) {
            mLogWarn("Exiting with signal {}", sig);
            std::exit(0);
        });

        // Gather a list of local ips so that we can detect connections from
        // the same machine.
        boost::asio::ip::tcp::resolver resolver(ctx);
        std::set<boost::asio::ip::address> local_ips = mobsya::network_interfaces_addresses();
        for(auto&& ip : local_ips) {
            mLogTrace("Local Ip : {}", ip.to_string());
        }

        // Create a server for regular tcp connection
        mobsya::application_server<mobsya::tcp::socket> tcp_server(ctx, 0);
        tcp_server.accept();

        mobsya::aseba_node_registery node_registery(ctx);
        node_registery.set_tcp_endpoint(tcp_server.endpoint());

        mLogTrace("=> TCP Server connected on {}", tcp_server.endpoint().port());
        mobsya::aseba_tcp_acceptor aseba_tcp_acceptor(ctx);
        // Create a server for websocket
        mobsya::application_server<mobsya::websocket_t> websocket_server(ctx, 8597);
        websocket_server.accept();
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

        ctx.run();
    } catch(boost::system::system_error e) {
        mLogError("Exception thrown: {}", e.what());
        std::exit(e.code().value());
    } catch(std::exception e) {
        mLogError("Exception thrown: {}", e.what());
        std::exit(1);
    }
    std::exit(0);
}
