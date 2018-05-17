#include <boost/program_options.hpp>
#include <boost/asio/executor_work_guard.hpp>
#include "usbserver.h"
#include "log.h"
#include "aseba_node_registery.h"

int main() {
    mLogInfo("Starting...");
    boost::program_options::options_description d("test");
    boost::asio::io_context ctx;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work(make_work_guard(ctx));

    mobsya::aseba_node_registery node_registery(ctx);
    mobsya::usb_server server(ctx, {{0x0617, 0x000a}});
    server.accept();
    ctx.run();
}
