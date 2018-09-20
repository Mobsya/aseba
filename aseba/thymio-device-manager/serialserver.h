#pragma once
#include "serialacceptor.h"
#include <memory>

namespace mobsya {
class serial_server {
public:
    serial_server(boost::asio::io_context& io_service, std::initializer_list<usb_device_identifier> = {});
    void accept();

private:
    boost::asio::io_context& m_io_ctx;
    serial_acceptor m_acceptor;
};
}  // namespace mobsya
