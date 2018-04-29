#pragma once
#include <libusb/libusb.h>
#include <boost/asio/io_service.hpp>
#include <boost/asio/basic_io_object.hpp>

namespace mobsya {

class usb_device_service : public boost::asio::detail::service_base<usb_device_service> {
public:
    enum class baud_rate {
        baud_1200 = 1200,
        baud_2400 = 2400,
        baud_4800 = 4800,
        baud_9600 = 9600,
        baud_19200 = 19200,
        baud_38400 = 34800,
        baud_57600 = 57600,
        baud_115200 = 115200
    };
    enum class data_bits { data_5 = 5, data_6 = 6, data_7 = 7, data_8 = 8 };
    enum class stop_bits { one = 1, two = 2, one_and_half = 3 };
    enum class parity { none = 0, even = 2, odd = 3, space = 4, mark = 5 };
    enum class flow_control { none = 0, hardware = 1, software = 2 };

    using native_handle_type = libusb_device*;

    usb_device_service(boost::asio::io_context& io_context);
    struct implementation_type {
        libusb_device* device = nullptr;
        libusb_device_handle* handle = nullptr;
        std::array<unsigned char, 7> control_line;
        bool dtr = true;
        uint8_t out_address = 0;
        uint8_t in_address = 0;
        int read_size = 0;
        int write_size = 0;
    };
    void construct(implementation_type&);
    void destroy(implementation_type&);

    void cancel(implementation_type& impl);
    void close(implementation_type& impl);
    bool is_open(implementation_type& impl);
    native_handle_type native_handle(implementation_type& impl);
    void open(implementation_type& impl);

    template <typename ConstBufferSequence, typename WriteHandler>
    BOOST_ASIO_INITFN_RESULT_TYPE(WriteHandler, void(boost::system::error_code, std::size_t))
    async_write_some(implementation_type& impl, const ConstBufferSequence& buffers, WriteHandler&& handler);

    template <typename MutableBufferSequence, typename ReadHandler>
    BOOST_ASIO_INITFN_RESULT_TYPE(ReadHandler, void(boost::system::error_code, std::size_t))
    async_read_some(implementation_type& impl, const MutableBufferSequence& buffers, ReadHandler&& handler);

    template <typename ConstBufferSequence>
    std::size_t write_some(implementation_type& impl, const ConstBufferSequence& buffers,
                           boost::system::error_code& ec);

    template <typename MutableBufferSequence>
    std::size_t read_some(implementation_type& impl, const MutableBufferSequence& buffers,
                          boost::system::error_code& ec);


    void assign(implementation_type& impl, libusb_device* d);
    void set_baud_rate(implementation_type& impl, baud_rate);
    void set_data_bits(implementation_type& impl, data_bits);
    void set_stop_bits(implementation_type& impl, stop_bits);
    void set_parity(implementation_type& impl, parity);
    void set_data_terminal_ready(implementation_type& impl, bool dtr);

private:
    bool send_control_transfer(implementation_type& impl);
    bool send_encoding(implementation_type& impl);
};


class usb_device : public boost::asio::basic_io_object<usb_device_service> {
public:
    using baud_rate = usb_device_service::baud_rate;
    using data_bits = usb_device_service::data_bits;
    using stop_bits = usb_device_service::stop_bits;
    using flow_control = usb_device_service::flow_control;
    using parity = usb_device_service::parity;

    using native_handle_type = libusb_device*;

    usb_device(boost::asio::io_context& io_context);
    void assign(native_handle_type);
    void cancel();
    void close();
    bool is_open();
    native_handle_type native_handle();
    void open();

    template <typename ConstBufferSequence, typename WriteHandler>
    BOOST_ASIO_INITFN_RESULT_TYPE(WriteHandler, void(boost::system::error_code, std::size_t))
    async_write_some(const ConstBufferSequence& buffers, WriteHandler&& handler) {
        return this->get_service().async_read_some(this->get_implementation(), buffers, std::move(handler));
    }

    template <typename MutableBufferSequence, typename ReadHandler>
    BOOST_ASIO_INITFN_RESULT_TYPE(ReadHandler, void(boost::system::error_code, std::size_t))
    async_read_some(const MutableBufferSequence& buffers, ReadHandler&& handler) {
        return this->get_service().async_read_some(this->get_implementation(), buffers, std::move(handler));
    }

    template <typename ConstBufferSequence>
    std::size_t write_some(const ConstBufferSequence& buffers, boost::system::error_code& ec) {
        return this->get_service().write_some(this->get_implementation(), buffers, ec);
    }

    template <typename MutableBufferSequence>
    std::size_t read_some(const MutableBufferSequence& buffers, boost::system::error_code& ec) {
        return this->get_service().read_some(this->get_implementation(), buffers, ec);
    }

    void set_baud_rate(baud_rate);
    void set_data_bits(data_bits);
    void set_stop_bits(stop_bits);
    void set_parity(parity);
    void set_data_terminal_ready(bool dtr);
};

}  // namespace mobsya
