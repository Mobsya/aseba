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

private:
    friend class serial_acceptor_service;
    usb_device_identifier m_device_id;
    std::string m_device_name;
};
}  // namespace mobsya
