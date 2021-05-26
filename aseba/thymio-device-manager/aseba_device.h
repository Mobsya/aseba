#pragma once
#include <boost/asio.hpp>
#include "usb_utils.h"
#ifdef MOBSYA_TDM_ENABLE_USB
#    include "usbdevice.h"
#endif
#ifdef MOBSYA_TDM_ENABLE_SERIAL
#    include "serial_usb_device.h"
#endif
#include "variant_compat.h"

namespace mobsya {

constexpr usb_device_identifier THYMIO2_DEVICE_ID = {0x0617, 0x000a};
constexpr usb_device_identifier THYMIO_WIRELESS_DEVICE_ID = {0x0617, 0x000c};

class aseba_device {


public:
    template <typename... Args>
    explicit aseba_device(Args&&... args) : m_endpoint(std::forward<Args>(args)...) {}
    aseba_device(aseba_device&&);
    ~aseba_device();

    using tcp_socket = boost::asio::ip::tcp::socket;
    using endpoint_t = variant_ns::variant<variant_ns::monostate
#ifdef MOBSYA_TDM_ENABLE_TCP
                                            ,
                                            tcp_socket
#endif
#ifdef MOBSYA_TDM_ENABLE_USB
                                           ,
                                           usb_device
#endif
#ifdef MOBSYA_TDM_ENABLE_SERIAL
                                           ,
                                           usb_serial_port
#endif
                                           >;
#ifdef MOBSYA_TDM_ENABLE_USB
    const usb_device& usb() const {
        return variant_ns::get<usb_device>(m_endpoint);
    }

    usb_device& usb() {
        return variant_ns::get<usb_device>(m_endpoint);
    }
#endif

#ifdef MOBSYA_TDM_ENABLE_SERIAL
    const usb_serial_port& serial() const {
        return variant_ns::get<usb_serial_port>(m_endpoint);
    }

    usb_serial_port& serial() {
        return variant_ns::get<usb_serial_port>(m_endpoint);
    }

#endif

#ifdef MOBSYA_TDM_ENABLE_TCP
    const tcp_socket& tcp() const {
        return variant_ns::get<tcp_socket>(m_endpoint);
    }

    tcp_socket& tcp() {
        return variant_ns::get<tcp_socket>(m_endpoint);
    }
#endif

    bool is_usb() const {
#ifdef MOBSYA_TDM_ENABLE_USB
        if(variant_ns::holds_alternative<usb_device>(m_endpoint)) {
            return variant_ns::get<usb_device>(m_endpoint).usb_device_id() == THYMIO2_DEVICE_ID;
        }
#endif
#ifdef MOBSYA_TDM_ENABLE_SERIAL
        if(variant_ns::holds_alternative<usb_serial_port>(m_endpoint)) {
            return variant_ns::get<usb_serial_port>(m_endpoint).usb_device_id() == THYMIO2_DEVICE_ID;
        }
#endif
        return false;
    }

    bool is_empty() const {
        return variant_ns::holds_alternative<variant_ns::monostate>(m_endpoint);
    }

    bool is_tcp() const {
#ifdef MOBSYA_TDM_ENABLE_TCP
        return variant_ns::holds_alternative<tcp_socket>(m_endpoint);
#else
        return false;
#endif
    }

    bool is_wireless() const {
#ifdef MOBSYA_TDM_ENABLE_USB
        if(variant_ns::holds_alternative<usb_device>(m_endpoint)) {
            return variant_ns::get<usb_device>(m_endpoint).usb_device_id() == THYMIO_WIRELESS_DEVICE_ID;
        }
#endif
#ifdef MOBSYA_TDM_ENABLE_SERIAL
        if(variant_ns::holds_alternative<usb_serial_port>(m_endpoint)) {
            return variant_ns::get<usb_serial_port>(m_endpoint).usb_device_id() == THYMIO_WIRELESS_DEVICE_ID;
        }
#endif
        return false;
    }

    endpoint_t& ep() {
        return m_endpoint;
    }

    boost::asio::executor get_executor() {
#ifdef MOBSYA_TDM_ENABLE_SERIAL
        if(variant_ns::holds_alternative<usb_serial_port>(m_endpoint)) {
            return variant_ns::get<usb_serial_port>(m_endpoint).get_executor();
        }
#endif
#ifdef MOBSYA_TDM_ENABLE_USB
        if(variant_ns::holds_alternative<usb_device>(m_endpoint)) {
            return variant_ns::get<usb_device>(m_endpoint).get_executor();
        }
#endif
#ifdef MOBSYA_TDM_ENABLE_TCP
        if (is_tcp()) {
            return variant_ns::get<tcp_socket>(m_endpoint).get_executor();
        }
#endif
        return boost::asio::executor();
    }

    void close();
    void start();
    void stop();
    void cancel_all_ops();
    void free_endpoint();
    void reboot();

    endpoint_t m_endpoint;
    bool m_active = false;
};

}  // namespace mobsya
