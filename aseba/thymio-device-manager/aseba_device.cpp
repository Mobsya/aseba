#include "aseba_device.h"
#ifdef MOBSYA_TDM_ENABLE_SERIAL
#    include "serialacceptor.h"
#endif
#ifdef MOBSYA_TDM_ENABLE_USB
#    include "usbacceptor.h"
#endif
#include "aseba_tcpacceptor.h"

namespace mobsya {


void aseba_device::cancel_all_ops() {
    boost::system::error_code ec;
    variant_ns::visit(overloaded{[](variant_ns::monostate&) {},
#ifdef MOBSYA_TDM_ENABLE_USB
                                 [](usb_device& underlying) { underlying.cancel(); },
#endif
                                 [&ec](auto& underlying) { underlying.cancel(ec); }},
                      m_endpoint);
}

aseba_device::~aseba_device() {
    free_endpoint();
}

aseba_device::aseba_device(aseba_device&& o) {
    swap(m_endpoint, o.m_endpoint);
}

void aseba_device::free_endpoint() {
    variant_ns::visit(
        overloaded{[](variant_ns::monostate&) {},
                   [this](tcp_socket&) {
                       boost::asio::post(get_executor(), [this, &ctx = get_executor().context()] {
                           boost::asio::use_service<aseba_tcp_acceptor>(ctx).free_endpoint(this);
                       });
                   }
#ifdef MOBSYA_TDM_ENABLE_SERIAL
                   ,
                   [this](mobsya::usb_serial_port& d) {
                       if(d.device_path().empty())
                           return;


                       auto timer = std::make_shared<boost::asio::deadline_timer>(
                           static_cast<boost::asio::io_context&>(get_executor().context()));
                       timer->expires_from_now(boost::posix_time::milliseconds(500));
                       timer->async_wait([timer, path = d.device_path(), &ctx = get_executor().context()](auto ec) {
                           if(!ec)
                               boost::asio::use_service<serial_acceptor_service>(ctx).free_device(path);
                       });
                   }
#endif
#ifdef MOBSYA_TDM_ENABLE_USB
                   ,
                   [this](mobsya::usb_device& d) {
                       if(d.native_handle()) {
                           boost::asio::post(
                               get_executor(),
                               [h = libusb_get_device(d.native_handle()), &ctx = get_executor().context()] {
                                   boost::asio::use_service<usb_acceptor_service>(ctx).free_device(h);
                               });
                       }
                   }
#endif
        },
        m_endpoint);
}

void aseba_device::stop() {
    variant_ns::visit(overloaded{[](variant_ns::monostate&) {}, [](tcp_socket& socket) {
                                     boost::system::error_code e;
                                     socket.cancel(e);
                                 }
#ifdef MOBSYA_TDM_ENABLE_SERIAL
                                 ,
                                 [](mobsya::usb_serial_port& d) {
                                     boost::system::error_code ec;
                                     d.cancel(ec);
                                 }
#endif
#ifdef MOBSYA_TDM_ENABLE_USB
                                 ,
                                 [](mobsya::usb_device& d) { d.cancel(); }
#endif
                      },
                      m_endpoint);
}

void aseba_device::close() {
    variant_ns::visit(overloaded{[](variant_ns::monostate&) {}, [](tcp_socket&) {}
#ifdef MOBSYA_TDM_ENABLE_SERIAL
                                 ,
                                 [](mobsya::usb_serial_port& d) { d.close(); }
#endif
#ifdef MOBSYA_TDM_ENABLE_USB
                                 ,
                                 [](mobsya::usb_device& d) { d.close(); }
#endif
                      },
                      m_endpoint);
}

}  // namespace mobsya
