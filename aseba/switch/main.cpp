#include <boost/program_options.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/thread.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <errno.h>
#include "log.h"
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

boost::interprocess::named_mutex unique_instance_lock(boost::interprocess::open_or_create, "mobsya-0accdcbf-eeb2");

void release_app_mutex() {
    static bool released = false;
    if(!released) {
        mLogInfo("Destroying mutex...");
        unique_instance_lock.unlock();
        released = true;
    }
}

int main() {
    mLogInfo("Starting...");
    boost::asio::io_context ctx;
    boost::asio::signal_set sig(ctx);
    try {

        boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work(make_work_guard(ctx));

        if(!unique_instance_lock.try_lock()) {
            mLogError("Thymio Device manager already running");
            return EALREADY;
        }

        // Make sure the lock is always released
        atexit(release_app_mutex);

        sig.add(SIGINT);
        sig.add(SIGTERM);
        sig.add(SIGABRT);
        sig.async_wait([](boost::system::error_code, int sig) {
            mLogWarn("Exiting with signal {}", sig);
            release_app_mutex();
            std::exit(0);
        });


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
