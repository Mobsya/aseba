#pragma once
#include <boost/asio/io_service.hpp>
#include <boost/asio/basic_io_object.hpp>
#include <libusb/libusb.h>
#include <queue>
#include <functional>
#include <mutex>
#include <thread>
#include "usbcontext.h"
#include "usbdevice.h"

namespace mobsya {

class usb_acceptor;
class usb_acceptor_service : public boost::asio::detail::service_base<usb_acceptor_service> {
public:
    usb_acceptor_service(boost::asio::io_context& io_service);
    struct implementation_type {
        std::vector<usb_device_identifier> compatible_devices;
    };
    void construct(implementation_type&);
    void destroy(implementation_type&);
    template <typename AcceptHandler>
    void accept_async(usb_acceptor& acceptor, usb_device& d, AcceptHandler&& handler) {
        request* ptr = nullptr;
        {
            std::unique_lock<std::mutex> _(m_req_mutex);
            request r{acceptor, d, std::function<void(boost::system::error_code)>(std::forward<AcceptHandler>(handler)),
                      0};
            m_requests.push(r);
            ptr = &m_requests.back();
        }
        register_request(*ptr);
    }
    void shutdown() override;

    int device_plugged(struct libusb_context* ctx, struct libusb_device* dev, libusb_hotplug_event event);
    friend int LIBUSB_CALL hotplug_callback(struct libusb_context* ctx, struct libusb_device* dev,
                                            libusb_hotplug_event event, void* user_data);

private:
    struct request {
        usb_acceptor& acceptor;
        usb_device& d;
        std::function<void(boost::system::error_code)> handler;
        int req_id;
    };
    void register_request(request&);
    details::usb_context::ptr m_context;
    libusb_hotplug_callback_handle m_cb_handle{};
    mutable std::mutex m_req_mutex;
    std::queue<request> m_requests;
};

class usb_acceptor : public boost::asio::basic_io_object<usb_acceptor_service> {
public:
    usb_acceptor(boost::asio::io_context& io_service, std::initializer_list<usb_device_identifier> = {});
    template <typename AcceptHandler>
    BOOST_ASIO_INITFN_RESULT_TYPE(AcceptHandler, void(boost::system::error_code))
    accept_async(usb_device& d, AcceptHandler&& handler) {
        boost::asio::async_completion<AcceptHandler, void(boost::system::error_code)> init(handler);
        this->get_service().accept_async(*this, d, std::forward<AcceptHandler>(init.completion_handler));
        return init.result.get();
    }
    const std::vector<usb_device_identifier>& compatible_devices() const;
};


}  // namespace mobsya
