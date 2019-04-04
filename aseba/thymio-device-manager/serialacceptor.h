#pragma once
#include <boost/asio/io_service.hpp>
#include <boost/asio/basic_io_object.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/serial_port.hpp>
#include <queue>
#include <functional>
#include <mutex>
#include <thread>
#include "usb_utils.h"
#include "serial_usb_device.h"
#ifdef MOBSYA_TDM_ENABLE_UDEV
#    include <libudev.h>
#    include <boost/asio/posix/stream_descriptor.hpp>
#endif

namespace mobsya {

class serial_acceptor;
class serial_acceptor_service : public boost::asio::detail::service_base<serial_acceptor_service> {
public:
    serial_acceptor_service(boost::asio::io_context& io_service);
    struct implementation_type {
        std::vector<usb_device_identifier> compatible_devices;
    };
    void construct(implementation_type&);
    void destroy(implementation_type&);
    template <typename AcceptHandler>
    void accept_async(serial_acceptor& acceptor, usb_serial_port& d, AcceptHandler&& handler) {
        request* ptr = nullptr;
        {
            std::unique_lock<std::mutex> _(m_req_mutex);
            request r{acceptor, d,
                      std::function<void(boost::system::error_code)>(std::forward<AcceptHandler>(handler))};
            m_requests.push(r);
            ptr = &m_requests.back();
        }
        register_request(*ptr);
    }
    void shutdown() override;

private:
    struct request {
        serial_acceptor& acceptor;
        usb_serial_port& d;
        std::function<void(boost::system::error_code)> handler;
    };
    void register_request(request&);
    mutable std::mutex m_req_mutex;
    std::queue<request> m_requests;
    boost::asio::strand<boost::asio::io_context::executor_type> m_strand;
    std::vector<std::string> m_known_devices;
#ifdef MOBSYA_TDM_ENABLE_UDEV
    struct udev* m_udev;
    struct udev_monitor* m_udev_monitor;
    boost::asio::posix::stream_descriptor m_desc;
    void on_udev_event(const boost::system::error_code&);
    bool handle_request(udev_device* dev, request& r);
    void async_wait();
#else
    boost::asio::deadline_timer m_active_timer;
    void handle_request_by_active_enumeration();
    void on_active_timer(const boost::system::error_code&);
#endif
};

class serial_acceptor : public boost::asio::basic_io_object<serial_acceptor_service> {
public:
    serial_acceptor(boost::asio::io_context& io_service, std::initializer_list<usb_device_identifier> = {});
    template <typename AcceptHandler>
    BOOST_ASIO_INITFN_RESULT_TYPE(AcceptHandler, void(boost::system::error_code))
    accept_async(usb_serial_port& d, AcceptHandler&& handler) {
        boost::asio::async_completion<AcceptHandler, void(boost::system::error_code)> init(handler);
        this->get_service().accept_async(*this, d, std::forward<AcceptHandler>(init.completion_handler));
        return init.result.get();
    }
    const std::vector<usb_device_identifier>& compatible_devices() const;
};


}  // namespace mobsya
