#pragma once
#include <boost/asio/serial_port.hpp>
#include "usb_utils.h"

namespace mobsya {
class usb_serial_port : public boost::asio::serial_port {
public:
    using serial_port::serial_port;
    usb_device_identifier usb_device_id() const {
        return m_device_id;
    }
    std::string device_name() const {
        return m_device_name;
    }

    void open() {
        boost::asio::serial_port::open(m_port_name);
    }
    void open(boost::system::error_code & ec) {
        boost::asio::serial_port::open(m_port_name, ec);
    }

    void set_data_terminal_ready(bool dtr) {
#ifdef _WIN32
        EscapeCommFunction(native_handle(), dtr ? SETDTR : CLRDTR);
#endif
    }
    void set_rts(bool rts) {
#ifdef _WIN32
        EscapeCommFunction(native_handle(), rts ? SETRTS : CLRRTS);
#endif
    }

private:
    friend class serial_acceptor_service;
    usb_device_identifier m_device_id;
    std::string m_device_name;
    std::string m_port_name;
};
}  // namespace mobsya
