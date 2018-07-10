#include "usbacceptor.h"
#include <algorithm>
#include <boost/asio/dispatch.hpp>
#include "log.h"
#include <libusb/libusb.h>

namespace mobsya {

int LIBUSB_CALL hotplug_callback(struct libusb_context* ctx, struct libusb_device* dev, libusb_hotplug_event event,
                                 void* user_data) {

    auto service = static_cast<usb_acceptor_service*>(user_data);
    return service->device_plugged(ctx, dev, event);
}

usb_acceptor_service::usb_acceptor_service(boost::asio::io_context& io_service)
    : boost::asio::detail::service_base<usb_acceptor_service>(io_service)
    , m_context(details::usb_context::acquire_context()) {

    mLogInfo("USB monitoring service: started");
}


void usb_acceptor_service::shutdown() {
    mLogInfo("USB monitoring service: Stopped");
    while(!m_requests.empty()) {
        libusb_hotplug_deregister_callback(*m_context, m_requests.front().req_id);
        m_requests.pop();
    }
    m_context = nullptr;
}


void usb_acceptor_service::register_request(request& r) {
    r.req_id =
        libusb_hotplug_register_callback(*m_context, /*context*/
                                         LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED /*events*/,
                                         LIBUSB_HOTPLUG_ENUMERATE /*flags */, LIBUSB_HOTPLUG_MATCH_ANY, /* vendor id */
                                         LIBUSB_HOTPLUG_MATCH_ANY,                                      /* product id */
                                         LIBUSB_HOTPLUG_MATCH_ANY /* class*/, hotplug_callback, this, &m_cb_handle);
}

int usb_acceptor_service::device_plugged(struct libusb_context* ctx, struct libusb_device* dev,
                                         libusb_hotplug_event event) {

    std::unique_lock<std::mutex> lock(m_req_mutex);
    if(m_requests.empty()) {
        return 0;
    }
    if(event != LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED) {
        return 0;
    }

    request& req = m_requests.front();
    struct libusb_device_descriptor desc;
    (void)libusb_get_device_descriptor(dev, &desc);

    //mLogTrace("device plugged : {}:{}", desc.idVendor, desc.idProduct);


    if(*m_context != ctx)
        return 0;

    const auto& devices = req.acceptor.compatible_devices();
    if(std::find(std::begin(devices), std::end(devices), usb_device_identifier{desc.idVendor, desc.idProduct}) ==
       std::end(devices)) {
        //mLogTrace("device not compatible : {}:{}", desc.idVendor, desc.idProduct);
        return 0;
    }

    if(m_context->is_device_open(dev)) {
        return 0;
    }

    req.d.assign(dev);
    libusb_hotplug_deregister_callback(*m_context, req.req_id);

    auto handler = std::move(req.handler);
    const auto executor = boost::asio::get_associated_executor(handler, req.acceptor.get_executor());
    m_requests.pop();
    boost::asio::post(executor, boost::beast::bind_handler(handler, boost::system::error_code{}));

    return 0;
}


void usb_acceptor_service::construct(implementation_type&) {}
void usb_acceptor_service::destroy(implementation_type&) {}

usb_acceptor::usb_acceptor(boost::asio::io_context& io_service, std::initializer_list<usb_device_identifier> devices)
    : boost::asio::basic_io_object<usb_acceptor_service>(io_service) {

    std::copy(std::begin(devices), std::end(devices),
              std::back_inserter(this->get_implementation().compatible_devices));
}

const std::vector<usb_device_identifier>& usb_acceptor::compatible_devices() const {
    return this->get_implementation().compatible_devices;
}


}  // namespace mobsya
