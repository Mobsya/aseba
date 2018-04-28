#pragma once
#include <libusb/libusb.h>
#include <boost/asio/io_service.hpp>
#include <boost/asio/basic_io_object.hpp>
#include "usbdevice.h"
#include <queue>
#include <functional>
#include <mutex>
#include <thread>

namespace mobsya {

namespace details {
    class usb_context {
    public:
        usb_context() {
            libusb_init(&ctx);
        }
        ~usb_context() {
            libusb_exit(ctx);
        }
        usb_context(const usb_context&) = delete;
        usb_context(usb_context&& other) {
            ctx = other.ctx;
            other.ctx = nullptr;
        }
        operator libusb_context*() const {
            return ctx;
        }

    private:
        libusb_context* ctx;
    };
}  // namespace details

struct usb_device_identifier {
    uint16_t vendor_id;
    uint16_t product_id;
};

inline bool operator==(const usb_device_identifier& a, const usb_device_identifier& b) {
    return a.vendor_id == b.vendor_id && a.product_id == b.product_id;
}

class usb_service : public boost::asio::detail::service_base<usb_service> {
public:
    usb_service(boost::asio::io_context& io_service);
    struct implementation_type {
        std::vector<usb_device_identifier> compatible_devices;
    };
    void construct(implementation_type&);
    void destroy(implementation_type&);
    template <typename AcceptHandler>
    void accept_async(implementation_type& impl, usb_device& d, AcceptHandler&& handler) {
        {
            std::unique_lock<std::mutex> _(m_req_mutex);
            m_requests.push(
                request{impl, d, std::function<void(boost::system::error_code)>(std::forward<AcceptHandler>(handler))});
        }
        start_thread();
    }
    void shutdown() override;

private:
    void start_thread();
    void async_wait_for_device();
    int device_plugged(struct libusb_context* ctx, struct libusb_device* dev, libusb_hotplug_event event);
    friend int hotplug_callback(struct libusb_context* ctx, struct libusb_device* dev, libusb_hotplug_event event,
                                void* user_data);

private:
    struct request {
        implementation_type& impl;
        usb_device& d;
        std::function<void(boost::system::error_code)> handler;
    };
    std::thread m_thread;
    details::usb_context m_context;
    libusb_hotplug_callback_handle m_cb_handle;
    std::atomic_bool m_running;
    mutable std::mutex m_req_mutex;
    std::queue<request> m_requests;
};

class usb_acceptor : public boost::asio::basic_io_object<usb_service> {
public:
    usb_acceptor(boost::asio::io_context& io_service, std::initializer_list<usb_device_identifier> = {});
    template <typename AcceptHandler>
    void accept_async(usb_device& d, AcceptHandler&& handler) {
        this->get_service().accept_async(this->get_implementation(), d, std::forward<AcceptHandler>(handler));
    }
};


}  // namespace mobsya
