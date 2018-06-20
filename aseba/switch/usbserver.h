#pragma once
#include "usbacceptor.h"
#include <memory>

namespace mobsya {

class usb_server {
public:
    usb_server(boost::asio::io_context& io_service, std::initializer_list<usb_device_identifier> = {});
    void accept();

private:
    boost::asio::io_context& m_io_ctx;
    usb_acceptor m_acceptor;
};
}  // namespace mobsya
