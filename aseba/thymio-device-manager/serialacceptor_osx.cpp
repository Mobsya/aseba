#include "serialacceptor.h"
#include <algorithm>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/beast/core/bind_handler.hpp>
#include "log.h"

#include <boost/scope_exit.hpp>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/serial/IOSerialKeys.h>
#include <IOKit/usb/USB.h>


namespace mobsya {

serial_acceptor_service::serial_acceptor_service(boost::asio::io_context& io_service)
    : boost::asio::detail::service_base<serial_acceptor_service>(io_service)
    , m_strand(io_service.get_executor())
    , m_active_timer(io_service) {

    mLogInfo("Serial monitoring service: started");
}


void serial_acceptor_service::shutdown() {
    mLogInfo("Serial monitoring service: Stopped");
    m_active_timer.cancel();
}


void serial_acceptor_service::register_request(request& r) {
    m_active_timer.expires_from_now(boost::posix_time::millisec(1));
    m_active_timer.async_wait(boost::asio::bind_executor(
        m_strand, boost::bind(&serial_acceptor_service::on_active_timer, this, boost::placeholders::_1)));
}

void serial_acceptor_service::free_device(const std::string& s) {
    auto it = std::find(m_known_devices.begin(), m_known_devices.end(), s);
    if(it != m_known_devices.end())
        m_known_devices.erase(it);
    remove_connected_device(s);
    m_active_timer.async_wait(boost::asio::bind_executor(
        m_strand, boost::bind(&serial_acceptor_service::handle_request_by_active_enumeration, this)));
}


void serial_acceptor_service::on_active_timer(const boost::system::error_code& ec) {
    if(ec)
        return;
    std::unique_lock<std::mutex> lock(m_req_mutex);
    this->handle_request_by_active_enumeration();
    if(!m_requests.empty()) {
        m_active_timer.expires_from_now(boost::posix_time::milliseconds(500));  // :(
        m_active_timer.async_wait(boost::asio::bind_executor(
            m_strand, boost::bind(&serial_acceptor_service::on_active_timer, this, boost::placeholders::_1)));
    }
}


static uint16_t property_as_short(io_registry_entry_t ioRegistryEntry, const char* propertyKey) {
    auto k = CFStringCreateWithCString(kCFAllocatorDefault, propertyKey, kCFStringEncodingUTF8);
    auto idx = IORegistryEntrySearchCFProperty(ioRegistryEntry, kIOServicePlane, k, kCFAllocatorDefault, 0);
    BOOST_SCOPE_EXIT(&k, &idx) {
        if(idx)
            CFRelease(idx);
        CFRelease(k);
    }
    BOOST_SCOPE_EXIT_END
    if(idx == nullptr)
        return 0;
    int16_t value = 0;
    CFNumberGetValue(CFNumberRef(idx), kCFNumberShortType, &value);
    return value;
}

static std::string property_as_string(io_registry_entry_t ioRegistryEntry, const char* propertyKey) {

    // get path for device
    auto k = CFStringCreateWithCString(kCFAllocatorDefault, propertyKey, kCFStringEncodingUTF8);
    CFTypeRef cfstr = IORegistryEntrySearchCFProperty(ioRegistryEntry, kIOServicePlane, k, kCFAllocatorDefault, 0);
    BOOST_SCOPE_EXIT(&k, &cfstr) {
        if(cfstr)
            CFRelease(cfstr);
        CFRelease(k);
    }
    BOOST_SCOPE_EXIT_END
    if(!cfstr || CFGetTypeID(cfstr) != CFStringGetTypeID()) {
        return {};
    }
    std::string str(CFStringGetCStringPtr(CFStringRef(cfstr), kCFStringEncodingUTF8));
    str.erase(std::find_if(str.rbegin(), str.rend(), [](int ch) { return !std::isspace(ch); }).base(), str.end());
    return str;
}

static io_registry_entry_t get_parent(io_registry_entry_t service) {
    io_registry_entry_t result = 0;
    ::IORegistryEntryGetParentEntry(service, kIOServicePlane, &result);
    ::IOObjectRelease(service);
    return result;
}

void serial_acceptor_service::handle_request_by_active_enumeration() {
    if(m_requests.empty() || m_paused)
        return;


    CFMutableDictionaryRef classesToMatch = IOServiceMatching(kIOSerialBSDServiceValue);
    if(classesToMatch == NULL)
        return;
    // specify all types of serial devices
    CFDictionarySetValue(classesToMatch, CFSTR(kIOSerialBSDTypeKey), CFSTR(kIOSerialBSDAllTypes));
    // get an iterator to serial port services
    io_iterator_t matchingServices;
    kern_return_t kernResult = IOServiceGetMatchingServices(kIOMasterPortDefault, classesToMatch, &matchingServices);
    if(KERN_SUCCESS != kernResult) {
        mLogError("IOServiceGetMatchingServices failed");
        return;
    }
    BOOST_SCOPE_EXIT(&matchingServices) {
        IOObjectRelease(matchingServices);
    }
    BOOST_SCOPE_EXIT_END

    std::vector<std::string> known_devices;
    const auto devices = m_requests.front().acceptor.compatible_devices();


    // iterate over services
    io_object_t service;
    while((service = IOIteratorNext(matchingServices))) {
        BOOST_SCOPE_EXIT(&service) {
            IOObjectRelease(service);
        }
        BOOST_SCOPE_EXIT_END
        usb_device_identifier id{0, 0};
        std::string name, path;
        do {
            if(id.vendor_id == 0)
                id.vendor_id = property_as_short(service, kUSBVendorID);
            if(id.product_id == 0)
                id.product_id = property_as_short(service, kUSBProductID);
            if(name.empty())
                name = property_as_string(service, kUSBProductString);
            if(path.empty())
                path = property_as_string(service, kIOCalloutDeviceKey);
        } while((id.product_id == 0 || id.vendor_id == 0 || name.empty() || path.empty()) &&
                (service = get_parent(service)));

        if(std::find(std::begin(devices), std::end(devices), id) == std::end(devices)) {
            // mLogTrace("device {} not compatible : {:#06X}-{:#06X} ", name, id.vendor_id, id.product_id);
            continue;
        }

        if(path.empty())
            continue;

        known_devices.push_back(path);

        if(m_requests.empty())
            continue;


        auto& req = m_requests.front();
        if(std::find(m_connected_devices.begin(), m_connected_devices.end(), path) != m_connected_devices.end())
            continue;

        mLogTrace("device : {:#06X}-{:#06X} on {}", id.vendor_id, id.product_id, path);
        boost::system::error_code ec;
        req.d.m_port_name = path;
        req.d.m_device_id = id;
        req.d.m_device_name = name;
        req.d.open(ec);
        if(!ec) {
            auto handler = std::move(req.handler);
            const auto executor = boost::asio::get_associated_executor(handler, req.acceptor.get_executor());
            m_requests.pop();
            boost::asio::post(executor, boost::beast::bind_handler(handler, boost::system::error_code{}));
            m_connected_devices.push_back(path);
            continue;
        }
    }
    for(auto&& e : m_known_devices) {
        if(std::find(known_devices.begin(), known_devices.end(), e) == known_devices.end()) {
            mLogError("Removed {}", e);
            remove_connected_device(e);
            device_unplugged(e);
        }
    }
    m_known_devices = known_devices;
}


void serial_acceptor_service::construct(implementation_type&) {}
void serial_acceptor_service::destroy(implementation_type&) {}

serial_acceptor::serial_acceptor(boost::asio::io_context& io_service,
                                 std::initializer_list<usb_device_identifier> devices)
    : boost::asio::basic_io_object<serial_acceptor_service>(io_service) {

    std::copy(std::begin(devices), std::end(devices),
              std::back_inserter(this->get_implementation().compatible_devices));
}

const std::vector<usb_device_identifier>& serial_acceptor::compatible_devices() const {
    return this->get_implementation().compatible_devices;
}


}  // namespace mobsya
