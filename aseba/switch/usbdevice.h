#pragma once
#include <libusb/libusb.h>
#include <boost/asio/io_service.hpp>
#include <boost/asio/basic_io_object.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/beast/core/bind_handler.hpp>
#include <boost/beast/core/detail/type_traits.hpp>
#include <boost/bind.hpp>
#include <memory>
#include "error.h"


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
        std::size_t read_size = 0;
        std::size_t write_size = 0;
    };
    void construct(implementation_type&);
    void destroy(implementation_type&);

    void cancel(implementation_type& impl);
    void close(implementation_type& impl);
    bool is_open(implementation_type& impl);
    native_handle_type native_handle(implementation_type& impl);

    tl::expected<void, boost::system::error_code> open(implementation_type& impl);


    template <typename ConstBufferSequence, typename WriteHandler>
    void async_write_some(implementation_type& impl, const ConstBufferSequence& buffers, WriteHandler&& handler) {
        async_transfer_some(impl, impl.out_address, buffers, std::forward<WriteHandler>(handler));
    }

    template <typename MutableBufferSequence, typename ReadHandler>
    void async_read_some(implementation_type& impl, const MutableBufferSequence& buffers, ReadHandler&& handler) {
        async_transfer_some(impl, impl.in_address, buffers, std::forward<ReadHandler>(handler));
    }

    template <typename ConstBufferSequence>
    tl::expected<std::size_t, boost::system::error_code> write_some(implementation_type& impl,
                                                                    const ConstBufferSequence& buffers);

    template <typename MutableBufferSequence>
    tl::expected<std::size_t, boost::system::error_code> read_some(implementation_type& impl,
                                                                   const MutableBufferSequence& buffers);

    std::size_t write_channel_chunk_size(const implementation_type& impl) const;
    std::size_t read_channel_chunk_size(const implementation_type& impl) const;

    void assign(implementation_type& impl, libusb_device* d);
    void set_baud_rate(implementation_type& impl, baud_rate);
    void set_data_bits(implementation_type& impl, data_bits);
    void set_stop_bits(implementation_type& impl, stop_bits);
    void set_parity(implementation_type& impl, parity);
    void set_data_terminal_ready(implementation_type& impl, bool dtr);

private:
    bool send_control_transfer(implementation_type& impl);
    bool send_encoding(implementation_type& impl);
};  // namespace mobsya


class usb_device : public boost::asio::basic_io_object<usb_device_service> {
public:
    using baud_rate = usb_device_service::baud_rate;
    using data_bits = usb_device_service::data_bits;
    using stop_bits = usb_device_service::stop_bits;
    using flow_control = usb_device_service::flow_control;
    using parity = usb_device_service::parity;

    using native_handle_type = libusb_device*;
    using lowest_layer_type = usb_device;

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

        static_assert(boost::asio::is_const_buffer_sequence<ConstBufferSequence>::value,
                      "ConstBufferSequence requirements not met");

        boost::asio::async_completion<WriteHandler, void(boost::system::error_code, std::size_t)> init(handler);
        this->async_transfer_some(this->get_implementation().out_address, buffers,
                                  std::forward<WriteHandler>(init.completion_handler));
        return init.result.get();
    }

    template <typename MutableBufferSequence, typename ReadHandler>
    BOOST_ASIO_INITFN_RESULT_TYPE(ReadHandler, void(boost::system::error_code, std::size_t))
    async_read_some(const MutableBufferSequence& buffers, ReadHandler&& handler) {

        static_assert(boost::asio::is_const_buffer_sequence<MutableBufferSequence>::value,
                      "MutableBufferSequence requirements not met");

        boost::asio::async_completion<ReadHandler, void(boost::system::error_code, std::size_t)> init(handler);
        this->async_transfer_some(this->get_implementation().in_address, buffers,
                                  std::forward<ReadHandler>(init.completion_handler));
        return init.result.get();
    }

    template <typename ConstBufferSequence>
    std::size_t write_some(const ConstBufferSequence& buffers) {
        auto r = this->get_service().write_some(this->get_implementation(), buffers);
        if(!r)
            boost::asio::detail::throw_error(r.error(), "write_some");
        return r.value();
    }

    template <typename ConstBufferSequence>
    std::size_t write_some(const ConstBufferSequence& buffers, boost::system::error_code& ec) {
        tl::expected<std::size_t, boost::system::error_code> r = this->write_some(buffers);
        if(r)
            return r.get();
        ec = r.error();
        return {};
    }

    template <typename MutableBufferSequence>
    std::size_t read_some(const MutableBufferSequence& buffers) {
        auto r = this->get_service().read_some(this->get_implementation(), buffers);
        if(!r)
            boost::asio::detail::throw_error(r.error(), "write_some");
        return r.value();
    }

    template <typename MutableBufferSequence>
    std::size_t read_some(const MutableBufferSequence& buffers, boost::system::error_code& ec) {
        tl::expected<std::size_t, boost::system::error_code> r = this->read_some(buffers);
        if(r)
            return r.get();
        ec = r.error();
        return {};
    }


    std::size_t write_channel_chunk_size() const;
    std::size_t read_channel_chunk_size() const;

    void set_baud_rate(baud_rate);
    void set_data_bits(data_bits);
    void set_stop_bits(stop_bits);
    void set_parity(parity);
    void set_data_terminal_ready(bool dtr);


    lowest_layer_type& lowest_layer() noexcept {
        return *this;
    }
    const lowest_layer_type& lowest_layer() const noexcept {
        return *this;
    }

private:
    template <typename BufferSequence, typename CompletionHandler>
    void async_transfer_some(uint8_t address, const BufferSequence& buffers, CompletionHandler&& handler);
};

template <typename ConstBufferSequence>
tl::expected<std::size_t, boost::system::error_code>
usb_device_service::write_some(implementation_type& impl, const ConstBufferSequence& buffers) {

    std::size_t total_writen = 0;
    for(const boost::asio::const_buffer& b : boost::beast::detail::buffers_range(buffers)) {
        int written = 0;
        auto err = libusb_bulk_transfer(impl.handle, impl.out_address,
                                        const_cast<unsigned char*>(static_cast<const unsigned char*>(b.data())),
                                        b.size(), &written, 10);
        if(err == LIBUSB_SUCCESS || err == LIBUSB_ERROR_TIMEOUT) {
            total_writen += written;
            if(err == LIBUSB_ERROR_TIMEOUT)
                break;
        } else {
            return usb::make_unexpected(err);
        }
    }
    return total_writen;
}

template <typename MutableBufferSequence>
tl::expected<std::size_t, boost::system::error_code>
usb_device_service::read_some(implementation_type& impl, const MutableBufferSequence& buffers) {
    std::size_t total_read = 0;
    for(const boost::asio::mutable_buffer& b : boost::beast::detail::buffers_range(buffers)) {
        int read = 0;
        auto err = libusb_bulk_transfer(impl.handle, impl.in_address, static_cast<unsigned char*>(b.data()), b.size(),
                                        &read, 10);
        if(err == LIBUSB_SUCCESS || err == LIBUSB_ERROR_TIMEOUT) {
            total_read += read;
            if(err == LIBUSB_ERROR_TIMEOUT)
                break;
        } else {
            return usb::make_unexpected(err);
        }
    }
    return total_read;
}

template <typename BufferSequence, typename CompletionHandler>
void usb_device::async_transfer_some(uint8_t address, const BufferSequence& buffers, CompletionHandler&& handler) {

    struct transfer_data;

    using data_allocator_t = typename std::allocator_traits<decltype(
        boost::asio::get_associated_allocator(handler))>::template rebind_alloc<transfer_data>;

    struct transfer_data {
        BufferSequence seq;
        usb_device& device;
        typename BufferSequence::const_iterator it;
        std::size_t total_transfered;
        CompletionHandler&& handler;
        data_allocator_t alloc;

        transfer_data(BufferSequence seq, usb_device& device, CompletionHandler&& handler)
            : seq(seq)
            , device(device)
            , it(boost::asio::buffer_sequence_begin(seq))
            , total_transfered(0)
            , handler(std::forward<CompletionHandler>(handler)) {}

        static void destroy(transfer_data* d) {
            auto alloc = std::move(d->alloc);
            std::allocator_traits<data_allocator_t>::destroy(alloc, d);
            std::allocator_traits<data_allocator_t>::deallocate(alloc, d, 1);
        }
        static auto unique_ptr(transfer_data* d) {
            return std::unique_ptr<transfer_data, decltype(&transfer_data::destroy)>(d, &transfer_data::destroy);
        }
        static auto allocate(data_allocator_t&& allocator, const BufferSequence& buffers, usb_device& device,
                             CompletionHandler&& handler) {
            transfer_data* data = allocator.allocate(1);
            std::allocator_traits<data_allocator_t>::construct(allocator, data, buffers, device,
                                                               std::forward<CompletionHandler>(handler));
            data->alloc = std::move(allocator);
            return std::unique_ptr<transfer_data, decltype(&transfer_data::destroy)>(data, &transfer_data::destroy);
        }
    };

    const auto& impl = this->get_implementation();

    using data_allocator_t = typename std::allocator_traits<decltype(
        boost::asio::get_associated_allocator(handler))>::template rebind_alloc<transfer_data>;
    data_allocator_t alloc(boost::asio::get_associated_allocator(handler));
    auto data = transfer_data::allocate(std::move(alloc), buffers, *this, std::forward<CompletionHandler>(handler));

    auto transfer = libusb_alloc_transfer(0);

    static auto cb = [](libusb_transfer* transfer) {
        auto d = transfer_data::unique_ptr(static_cast<transfer_data*>(transfer->user_data));

        d->total_transfered += transfer->actual_length;


        // Successful transfer and full buffer => read some more in the next buffer
        if(transfer->status == LIBUSB_TRANSFER_COMPLETED && transfer->actual_length == transfer->length &&
           d->it++ != std::end(d->seq)) {
            transfer->actual_length = 0;
            transfer->buffer = (decltype(transfer->buffer))d->it->data();
            transfer->length = d->it->size();
            libusb_submit_transfer(transfer);
            d.release();
            return;
        }
        boost::system::error_code ec;
        if(transfer->status != LIBUSB_TRANSFER_COMPLETED) {
            auto ec = usb::make_error_code_from_transfer(transfer->status);
        }

        libusb_free_transfer(transfer);

        auto handler = std::move(d->handler);
        const auto executor = boost::asio::get_associated_executor(handler, d->device.get_executor());
        const auto total_transfered = d->total_transfered;

        // Reset storage before posting
        d.reset();

        boost::asio::post(executor,
                          boost::beast::bind_handler(std::forward<CompletionHandler>(handler), ec, total_transfered));
    };

    libusb_fill_bulk_transfer(transfer, impl.handle, address, decltype(transfer->buffer)(data->it->data()),
                              data->it->size(), cb, data.get(), 0);
    auto r = libusb_submit_transfer(transfer);
    if(r != LIBUSB_SUCCESS) {
        libusb_free_transfer(transfer);
        boost::asio::post(
            boost::asio::get_associated_executor(handler, this->get_executor()),
            boost::beast::bind_handler(std::forward<CompletionHandler>(handler), usb::make_error_code(r), 0));
    } else {
        data.release();
    }

}  // namespace mobsya

}  // namespace mobsya
