#include "usbservice.h"
#include <libusb/libusb.h>
#include <algorithm>
#include <boost/asio/dispatch.hpp>
#include "log.h"

namespace mobsya {

int hotplug_callback(struct libusb_context* ctx, struct libusb_device* dev, libusb_hotplug_event event,
                     void* user_data) {

    auto service = static_cast<usb_service*>(user_data);
    return service->device_plugged(ctx, dev, event);
}

usb_service::usb_service(boost::asio::io_context& io_service)
    : boost::asio::detail::service_base<usb_service>(io_service), m_running(false) {}


void usb_service::start_thread() {
    if(!m_running) {
        m_thread = std::thread([this]() { async_wait_for_device(); });
    }
}

void usb_service::shutdown() {
    m_running = false;
    if(m_thread.joinable()) {
        m_thread.join();
        mLogDebug("libusb monitor thread stopped");
    }
}

void usb_service::async_wait_for_device() {
    mLogDebug("libusb monitor thread started");
    auto rc =
        libusb_hotplug_register_callback(m_context, /*context*/
                                         LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED /*events*/,
                                         LIBUSB_HOTPLUG_ENUMERATE /*flags */, LIBUSB_HOTPLUG_MATCH_ANY, /* vendor id */
                                         LIBUSB_HOTPLUG_MATCH_ANY,                                      /* product id */
                                         LIBUSB_HOTPLUG_MATCH_ANY /* class*/, hotplug_callback, this, &m_cb_handle);
    m_running = true;
    while(m_running) {
        timeval v = {0, 0};
        libusb_handle_events_timeout(m_context, &v);
    }
    libusb_hotplug_deregister_callback(m_context, rc);
}


int usb_service::device_plugged(struct libusb_context* ctx, struct libusb_device* dev, libusb_hotplug_event event) {

    std::unique_lock<std::mutex> lock(m_req_mutex);
    if(m_requests.empty()) {
        return 0;
    }
    if(event != LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED) {
        return 0;
    }

    request req = m_requests.front();
    struct libusb_device_descriptor desc;
    (void)libusb_get_device_descriptor(dev, &desc);

    mLogTrace("device plugged : {}:{}", desc.idVendor, desc.idProduct);


    if(m_context != ctx)
        return 0;

    if(std::find(std::begin(req.impl.compatible_devices), std::end(req.impl.compatible_devices),
                 usb_device_identifier{desc.idVendor, desc.idProduct}) == std::end(req.impl.compatible_devices)) {

        mLogTrace("device not compatible : {}:{}", desc.idVendor, desc.idProduct);
        return 0;
    }

    m_requests.pop();
    req.d.assign(dev);
    // Fixme, support for allocators, executors, etc
    boost::asio::dispatch(this->get_io_context(), [req]() { req.handler({}); });
    return 0;
}


void usb_service::construct(implementation_type&) {}
void usb_service::destroy(implementation_type&) {}

usb_acceptor::usb_acceptor(boost::asio::io_context& io_service, std::initializer_list<usb_device_identifier> devices)
    : boost::asio::basic_io_object<usb_service>(io_service) {

    std::copy(std::begin(devices), std::end(devices),
              std::back_inserter(this->get_implementation().compatible_devices));
}


}  // namespace mobsya
