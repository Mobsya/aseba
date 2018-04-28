#pragma once
#include "usbservice.h"
#include <memory>

namespace mobsya {

class usb_connection : public std::enable_shared_from_this<usb_connection> {
public:
    usb_device& device() {
        return m_device;
    }


private:
    usb_device m_device;
};


class usb_server {
public:
    usb_server(boost::asio::io_context& io_service, std::initializer_list<usb_device_identifier> = {});
    void accept();

private:
    boost::asio::io_context& m_io_ctx;
    usb_acceptor m_acceptor;
};
}  // namespace mobsya
