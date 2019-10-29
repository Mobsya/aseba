#include "serialacceptor.h"
#include <algorithm>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/beast/core/bind_handler.hpp>
#include <boost/thread/thread.hpp>
#include <boost/scope_exit.hpp>
#include "log.h"

namespace mobsya {

serial_acceptor_service::serial_acceptor_service(boost::asio::io_context& io_service)
    : boost::asio::detail::service_base<serial_acceptor_service>(io_service)
    , m_strand(io_service.get_executor())
    , m_udev(udev_new())
    , m_udev_monitor(nullptr)
    , m_desc(io_service)
    , m_active_timer(io_service) {

    mLogInfo("Serial monitoring service: started");
}


void serial_acceptor_service::shutdown() {
    udev_monitor_unref(m_udev_monitor);
    udev_unref(m_udev);
    mLogInfo("Serial monitoring service: Stopped");
}

void serial_acceptor_service::free_device(const std::string& s) {

    std::unique_lock<std::mutex> _(m_req_mutex);
    auto it = std::find(m_known_devices.begin(), m_known_devices.end(), s);
    if(it != m_known_devices.end()) {
        m_known_devices.erase(it);
    }
    remove_connected_device(s);

    m_active_timer.expires_from_now(boost::posix_time::milliseconds(500));
    m_active_timer.async_wait(boost::asio::bind_executor(
        m_strand, boost::bind(&serial_acceptor_service::handle_request_by_active_enumeration, this)));
}


bool serial_acceptor_service::handle_request(udev_device* dev, request& r) {
    std::unique_lock<std::mutex> _(m_req_mutex);
    if(m_requests.empty() || m_paused)
        return false;
    if(!dev) {
        mLogError("Fail to list null device");
        return false;
    }

    auto n = udev_device_get_devnode(dev);
    if(!n)
        return false;

    if(std::find(m_connected_devices.begin(), m_connected_devices.end(), n) != m_connected_devices.end()) {
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
    const auto id = usb_device_identifier{uint16_t(std::stoi(v, nullptr, 16)), uint16_t(std::stoi(p, nullptr, 16))};

    auto& compat = r.acceptor.compatible_devices();
    if(std::find(compat.begin(), compat.end(), id) == compat.end()) {
        mLogError("Not compatible");
        return false;
    }

    m_known_devices.push_back(n);

    r.d.m_device_name = s;
    r.d.m_port_name = n;
    r.d.m_device_id = id;

    m_connected_devices.push_back(n);

    auto handler = std::move(r.handler);
    const auto executor = boost::asio::get_associated_executor(handler, r.acceptor.get_executor());
    m_requests.pop();
    boost::asio::post(executor, boost::beast::bind_handler(handler, boost::system::error_code{}));
    return true;
}

void serial_acceptor_service::register_request(request&) {
    if(!m_udev)
        return;

    m_active_timer.expires_from_now(boost::posix_time::milliseconds(500));
    m_active_timer.async_wait(boost::asio::bind_executor(
        m_strand, boost::bind(&serial_acceptor_service::handle_request_by_active_enumeration, this)));

    if(!m_udev_monitor) {
        m_udev_monitor = udev_monitor_new_from_netlink(m_udev, "udev");
        udev_monitor_filter_add_match_subsystem_devtype(m_udev_monitor, "tty", nullptr);
        udev_monitor_enable_receiving(m_udev_monitor);
        auto fd = udev_monitor_get_fd(m_udev_monitor);
        m_desc.assign(fd);
        async_wait();
    }
}

void serial_acceptor_service::handle_request_by_active_enumeration() {
    if(m_requests.empty() || m_paused)
        return;

    std::set<std::string> known_devices;

    // Get initial list of devices
    auto enumerate = udev_enumerate_new(m_udev);
    udev_enumerate_add_match_subsystem(enumerate, "tty");
    udev_enumerate_scan_devices(enumerate);
    auto devices = udev_enumerate_get_list_entry(enumerate);
    struct udev_list_entry* entry;
    udev_list_entry_foreach(entry, devices) {
        auto path = udev_list_entry_get_name(entry);
        auto dev = udev_device_new_from_syspath(m_udev, path);
        auto n = udev_device_get_devnode(dev);
        if(n)
            known_devices.insert(n);
        try {
            BOOST_SCOPE_EXIT(dev) {
                udev_device_unref(dev);
            }
            BOOST_SCOPE_EXIT_END
            if(handle_request(dev, m_requests.front())) {
                break;
            }
        } catch(...) {
        }
    };
    udev_enumerate_unref(enumerate);


    std::unique_lock<std::mutex> _(m_req_mutex);
    for(auto it = m_known_devices.begin(); it != m_known_devices.end();) {
        if(known_devices.find(*it) == known_devices.end()) {
            mLogError("Removed {}", *it);
            device_unplugged(*it);
            remove_connected_device(*it);
            it = m_known_devices.erase(it);
        } else {
            ++it;
        }
    }
    async_wait();
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
    handle_request_by_active_enumeration();
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
