#pragma once
#include "serialacceptor.h"
#include <memory>

namespace mobsya {
class aseba_device;
class serial_server {
public:
    serial_server(boost::asio::io_context& io_service, std::initializer_list<usb_device_identifier> = {});
    void accept();

private:
    void register_configurable_dongle(usb_serial_port&& d) const;
    bool should_open_for_configuration(const usb_serial_port& d) const;
    boost::asio::io_context& m_io_ctx;
    serial_acceptor m_acceptor;
};
}  // namespace mobsya
