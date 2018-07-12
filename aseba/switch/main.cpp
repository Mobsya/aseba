#include <boost/program_options.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include "usbserver.h"
#include "log.h"
#include "aseba_node_registery.h"
#include "app_server.h"

int main() {
    mLogInfo("Starting...");
    boost::asio::io_context ctx;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work(make_work_guard(ctx));

    // Create a server for regular tcp connection
    mobsya::application_server<mobsya::tcp::socket> tcp_server(ctx, 0);

    // Create a server for websocket
    mobsya::application_server<mobsya::websocket_t> websocket_server(ctx, 8597);
    websocket_server.accept();


    mobsya::usb_server usb_server(ctx, {
                                  {0x0617, 0x000a},
                                  {0x0617, 0x000c}
                                 });


    mobsya::aseba_node_registery node_registery(ctx);
    node_registery.set_tcp_endpoint(tcp_server.endpoint());

    usb_server.accept();
    ctx.run();
}
