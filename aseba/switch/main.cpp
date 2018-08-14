#include <boost/program_options.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include "log.h"
#include "aseba_node_registery.h"
#include "app_server.h"
#include "aseba_endpoint.h"
#include "aseba_tcpacceptor.h"

#ifdef MOBSYA_TDM_ENABLE_USB
#include "usbserver.h"
#endif

int main() {
    mLogInfo("Starting...");
    boost::asio::io_context ctx;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work(make_work_guard(ctx));

    // Create a server for regular tcp connection
    mobsya::application_server<mobsya::tcp::socket> tcp_server(ctx, 0);

    // Create a server for websocket
    mobsya::application_server<mobsya::websocket_t> websocket_server(ctx, 8597);
    websocket_server.accept();

    mobsya::aseba_tcp_acceptor aseba_tcp_acceptor(ctx);


    mobsya::aseba_node_registery node_registery(ctx);
    node_registery.set_tcp_endpoint(tcp_server.endpoint());

#ifdef MOBSYA_TDM_ENABLE_USB
	mobsya::usb_server usb_server(ctx, { mobsya::THYMIO2_DEVICE_ID, mobsya::THYMIO_WIRELESS_DEVICE_ID }); 
	usb_server.accept();
#endif
    aseba_tcp_acceptor.accept();

    ctx.run();
}
