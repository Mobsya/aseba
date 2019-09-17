#include "usbacceptor.h"
#include <algorithm>
#include <boost/asio.hpp>
#include <libusb/libusb.h>
#include "log.h"

namespace mobsya {

usb_acceptor_service::usb_acceptor_service(boost::asio::io_context& io_service)
    : boost::asio::detail::service_base<usb_acceptor_service>(io_service)
    , m_context(details::usb_context::acquire_context())
    , m_active_timer(io_service)
    , m_strand(io_service.get_executor()) {

    mLogInfo("USB monitoring service: started");
}


void usb_acceptor_service::shutdown() {
    mLogInfo("USB monitoring service: Stopped");
    m_active_timer.cancel();
    while(!m_requests.empty()) {
        if(m_requests.front().req_id != -1)
            libusb_hotplug_deregister_callback(*m_context, m_requests.front().req_id);
        m_requests.pop();
    }
    m_context = nullptr;
}

void usb_acceptor_service::free_device(const libusb_device* dev) {
    auto it = std::find(m_known_devices.begin(), m_known_devices.end(), dev);
    if(it != m_known_devices.end())
        m_known_devices.erase(it);
    m_active_timer.expires_from_now(boost::posix_time::milliseconds(500));
    m_active_timer.async_wait(boost::asio::bind_executor(
        m_strand, boost::bind(&usb_acceptor_service::handle_request_by_active_enumeration, this)));
}

void usb_acceptor_service::register_request(request&) {
    m_active_timer.expires_from_now(boost::posix_time::seconds(1));
    m_active_timer.async_wait(boost::asio::bind_executor(
        m_strand, boost::bind(&usb_acceptor_service::on_active_timer, this, boost::placeholders::_1)));
}

void usb_acceptor_service::on_active_timer(const boost::system::error_code& ec) {
    if(ec)
        return;
    std::unique_lock<std::mutex> lock(m_req_mutex);
    this->handle_request_by_active_enumeration();
    if(!m_requests.empty()) {
        m_active_timer.expires_from_now(boost::posix_time::seconds(1));  // :(
        m_active_timer.async_wait(boost::asio::bind_executor(
            m_strand, boost::bind(&usb_acceptor_service::on_active_timer, this, boost::placeholders::_1)));
    }
}

int usb_acceptor_service::device_plugged(struct libusb_context* ctx, struct libusb_device* dev, request& req) {
    libusb_device_descriptor desc;
    (void)libusb_get_device_descriptor(dev, &desc);

    if(m_context->is_device_open(dev)) {
        return 0;
    }

    if(*m_context != ctx)
        return 0;

    // mLogTrace("device plugged : {}:{}", desc.idVendor, desc.idProduct);

    const auto& devices = req.acceptor.compatible_devices();
    if(std::find(std::begin(devices), std::end(devices), usb_device_identifier{desc.idVendor, desc.idProduct}) ==
       std::end(devices)) {
        // mLogTrace("device not compatible : {}:{}", desc.idVendor, desc.idProduct);
        return 0;
    }

    req.d.assign(dev);
    auto handler = std::move(req.handler);
    const auto executor = boost::asio::get_associated_executor(handler, req.acceptor.get_executor());
    m_requests.pop();
    boost::asio::post(executor, boost::beast::bind_handler(handler, boost::system::error_code{}));
    return 1;
}


void usb_acceptor_service::handle_request_by_active_enumeration() {
    libusb_device** devices;
    ssize_t ndevices = libusb_get_device_list(*m_context, &devices);
    std::vector<libusb_device*> sessions;
    sessions.reserve(ndevices);

    for(ssize_t i = 0; !m_requests.empty() && ndevices > 0 && i < ndevices; i++) {
        sessions.push_back(devices[i]);
        if(std::find(m_known_devices.begin(), m_known_devices.end(), devices[i]) == m_known_devices.end()) {
            device_plugged(*m_context, devices[i], m_requests.front());
        }
    }
    libusb_free_device_list(devices, true);

    for(auto&& d : m_known_devices) {
        if(std::find(sessions.begin(), sessions.end(), d) == sessions.end()) {
            device_unplugged(d);
        }
    }
    m_known_devices = std::move(sessions);
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
