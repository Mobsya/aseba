#pragma once
#include "usbacceptor.h"
#include <memory>

namespace mobsya {
class aseba_device;
class usb_server {
public:
    usb_server(boost::asio::io_context& io_service, std::initializer_list<usb_device_identifier> = {});
    void accept();

private:
    void register_configurable_dongle(aseba_device&& d) const;
    bool should_open_for_configuration(const usb_device& d) const;
    boost::asio::io_context& m_io_ctx;
    usb_acceptor m_acceptor;
};
}  // namespace mobsya
