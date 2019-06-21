#pragma once
#include <boost/asio/serial_port.hpp>
#include "usb_utils.h"
#if defined(__linux__) or defined(__APPLE__)
#    include <sys/ioctl.h>
#endif
namespace mobsya {
class usb_serial_port : public boost::asio::serial_port {
public:
    using boost::asio::serial_port::serial_port;
    usb_device_identifier usb_device_id() const {
        return m_device_id;
    }

    std::string device_path() const {
        return m_port_name;
    }

    std::string device_name() const {
        return m_device_name;
    }
    void open() {
        boost::system::error_code ec;
        open(ec);
    }
    void open(boost::system::error_code& ec) {
        boost::asio::serial_port::open(m_port_name, ec);
#if defined(__linux__) or defined(__APPLE__)
        ioctl(native_handle(), _IO('U', 20), 0);
        ioctl(native_handle(), TIOCEXCL);
        tcflush(native_handle(), TCIOFLUSH);
#endif
    }

    void close() {
#if defined(__linux__) or defined(__APPLE__)
        ioctl(native_handle(), _IO('U', 20), 0);
        ioctl(native_handle(), TIOCEXCL);
        tcflush(native_handle(), TCIOFLUSH);
#endif
        boost::system::error_code ec;
        close(ec);
    }
    void close(boost::system::error_code& ec) {
        boost::asio::serial_port::close(ec);
    }

    void set_data_terminal_ready(bool dtr) {
#ifdef _WIN32
        EscapeCommFunction(native_handle(), dtr ? SETDTR : CLRDTR);
#endif
#if defined(__linux__) or defined(__APPLE__)
        int flag = TIOCM_DTR;
        ioctl(native_handle(), dtr ? TIOCMBIS : TIOCMBIC, &flag);
#endif
    }
    void set_rts(bool rts) {
#ifdef _WIN32
        EscapeCommFunction(native_handle(), rts ? SETRTS : CLRRTS);
#endif
#if defined(__linux__) or defined(__APPLE__)
        int flag = TIOCM_RTS;
        ioctl(native_handle(), rts ? TIOCMBIS : TIOCMBIC, &flag);
#endif
    }

    void purge() {
#ifdef _WIN32
        PurgeComm(native_handle(), PURGE_RXABORT | PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT);
#endif
    }

private:
    friend class serial_acceptor_service;
    usb_device_identifier m_device_id;
    std::string m_device_name;
    std::string m_port_name;
};
}  // namespace mobsya
