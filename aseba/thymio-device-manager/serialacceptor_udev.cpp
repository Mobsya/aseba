#include "serialacceptor.h"
#include <algorithm>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/beast/core/bind_handler.hpp>
#include "log.h"

namespace mobsya {

serial_acceptor_service::serial_acceptor_service(boost::asio::io_context& io_service)
    : boost::asio::detail::service_base<serial_acceptor_service>(io_service)
    , m_udev(udev_new())
    , m_udev_monitor(nullptr)
    , m_desc(io_service)
    , m_strand(io_service.get_executor())
    , m_active_timer(io_service) {

    mLogInfo("Serial monitoring service: started");
}


void serial_acceptor_service::shutdown() {
    udev_monitor_unref(m_udev_monitor);
    udev_unref(m_udev);
    mLogInfo("Serial monitoring service: Stopped");
}

void serial_acceptor_service::free_device(const std::string& s) {
    m_known_devices.erase(std::find(m_known_devices.begin(), m_known_devices.end(), s));
    m_active_timer.expires_from_now(boost::posix_time::milliseconds(500));
    m_active_timer.async_wait(boost::asio::bind_executor(
        m_strand, boost::bind(&serial_acceptor_service::handle_request_by_active_enumeration, this)));
}


bool serial_acceptor_service::handle_request(udev_device* dev, request& r) {
    if(m_requests.empty() || m_paused)
        return false;
    if(!dev) {
        mLogError("Fail to list null device");
        return false;
    }

    if(std::find(m_known_devices.begin(), m_known_devices.end(), udev_device_get_syspath(dev)) !=
       m_known_devices.end()) {
        return false;
    }

    auto usb = udev_device_get_parent_with_subsystem_devtype(dev, "usb", "usb_device");
    if(!usb) {
        return false;
    }
    auto p = udev_device_get_sysattr_value(usb, "idProduct");
    auto v = udev_device_get_sysattr_value(usb, "idVendor");
    std::string s = udev_device_get_sysattr_value(usb, "product");
    if(!p || !v) {
        mLogError("Fail to list device : unable to extract id");
        return false;
    }
    const auto id = usb_device_identifier{uint16_t(std::stoi(v, 0, 16)), uint16_t(std::stoi(p, 0, 16))};

    auto& compat = r.acceptor.compatible_devices();
    if(std::find(compat.begin(), compat.end(), id) == compat.end()) {
        mLogError("Not compatible");
        return false;
    }


    boost::system::error_code ec;
    auto n = udev_device_get_devnode(dev);
    if(!n)
        return false;

    r.d.m_device_name = s;
    r.d.m_port_name = n;
    r.d.m_device_id = id;
    r.d.open(ec);


    if(ec) {

        mLogError("serial acceptor: {}", ec.message());
        return false;
    }
    auto handler = std::move(r.handler);
    const auto executor = boost::asio::get_associated_executor(handler, r.acceptor.get_executor());
    m_requests.pop();
    boost::asio::post(executor, boost::beast::bind_handler(handler, boost::system::error_code{}));
    m_known_devices.push_back(udev_device_get_syspath(dev));
    return true;
}

void serial_acceptor_service::register_request(request& r) {
    if(!m_udev)
        return;

    m_active_timer.expires_from_now(boost::posix_time::milliseconds(500));
    m_active_timer.async_wait(boost::asio::bind_executor(
        m_strand, boost::bind(&serial_acceptor_service::handle_request_by_active_enumeration, this)));

    if(!m_udev_monitor) {
        m_udev_monitor = udev_monitor_new_from_netlink(m_udev, "udev");
        udev_monitor_filter_add_match_subsystem_devtype(m_udev_monitor, "tty", NULL);
        udev_monitor_enable_receiving(m_udev_monitor);
        auto fd = udev_monitor_get_fd(m_udev_monitor);
        m_desc.assign(fd);
        async_wait();
    }
}

void serial_acceptor_service::handle_request_by_active_enumeration() {
    if(m_requests.empty() || m_paused)
        return;


    // Get initial list of devices
    auto enumerate = udev_enumerate_new(m_udev);
    udev_enumerate_add_match_subsystem(enumerate, "tty");
    udev_enumerate_scan_devices(enumerate);
    auto devices = udev_enumerate_get_list_entry(enumerate);
    struct udev_list_entry *entry, *attr;
    udev_list_entry_foreach(entry, devices) {
        auto path = udev_list_entry_get_name(entry);
        auto dev = udev_device_new_from_syspath(m_udev, path);
        try {
            if(handle_request(dev, m_requests.front())) {
                udev_enumerate_unref(enumerate);
                return;
            }
        } catch(...) {
        }
    };
    udev_enumerate_unref(enumerate);
}

void serial_acceptor_service::async_wait() {
    m_desc.async_wait(
        boost::asio::posix::descriptor::wait_read,
        boost::asio::bind_executor(
            m_strand, boost::bind(&serial_acceptor_service::on_udev_event, this, boost::placeholders::_1)));
}

void serial_acceptor_service::on_udev_event(const boost::system::error_code& ec) {
    if(ec)
        return;
    auto dev = udev_monitor_receive_device(m_udev_monitor);
    if(dev) {
        if(strcmp("add", udev_device_get_action(dev)) != 0) {
            return async_wait();
        }
        handle_request(dev, m_requests.front());
        udev_device_unref(dev);
        async_wait();
    }
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
