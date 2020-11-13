#pragma once
#include <boost/asio/io_service.hpp>
#include <boost/asio/basic_io_object.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/beast/core/bind_handler.hpp>
#include <boost/beast/core/detail/type_traits.hpp>
#include <boost/beast/core.hpp>
#include <boost/bind/bind.hpp>
#include <map>

// Include asio before libusb !
#include <libusb/libusb.h>
#include <memory>
#include "error.h"
#include "usbcontext.h"
#include "log.h"
#include <numeric>
#include <queue>

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

    using native_handle_type = libusb_device_handle*;

    usb_device_service(boost::asio::io_context& io_context);
    void shutdown() override;


    class buffer {
    public:
        buffer() : s(0) {}
        void reserve(std::size_t size, std::size_t buffer_size) {
            buffer_size = std::max(50 * buffer_size, std::size_t{1});
            v.resize(((s + size + buffer_size - 1) / buffer_size) * buffer_size);
        }


        void read(const boost::asio::mutable_buffer& b) {
            const auto n = std::min(b.size(), v.size());
            std::copy(v.data(), v.data() + n, static_cast<uint8_t*>(b.data()));
            auto tmp = std::vector<uint8_t>(v.begin() + n, v.end());
            v = std::move(tmp);
            s -= n;
        }

        uint8_t* write_begin() {
            return v.data() + s;
        }

        std::size_t write_capacity() const {
            return v.size() - s;
        }

        std::size_t size() const {
            return s;
        }

        void commit(std::size_t n) {
            s += n;
        }

    private:
        std::vector<uint8_t> v;
        std::size_t s;
    };


    struct implementation_type {
        libusb_device* device = nullptr;
        libusb_device_handle* handle = nullptr;
        std::array<unsigned char, 7> control_line;
        bool dtr = true;
        bool rts = false;
        uint8_t out_address = 0;
        uint8_t in_address = 0;
        std::size_t read_size = 0;
        std::size_t write_size = 0;
        buffer read_buffer;
        std::vector<libusb_transfer*> transfers;
        std::map<std::shared_ptr<buffer>, bool> buffers_pool;

        implementation_type() = default;

        implementation_type(implementation_type&& o) {
            std::swap(device, o.device);
            std::swap(handle, o.handle);
            std::swap(control_line, o.control_line);
            std::swap(dtr, o.rts);
            std::swap(out_address, o.out_address);
            std::swap(in_address, o.in_address);
            std::swap(read_size, o.read_size);
            std::swap(write_size, o.write_size);
            std::swap(read_buffer, o.read_buffer);
            std::swap(transfers, o.transfers);
            std::swap(buffers_pool, o.buffers_pool);
        }

        implementation_type& operator=(implementation_type&& o) {
            std::swap(device, o.device);
            std::swap(handle, o.handle);
            std::swap(control_line, o.control_line);
            std::swap(dtr, o.rts);
            std::swap(out_address, o.out_address);
            std::swap(in_address, o.in_address);
            std::swap(read_size, o.read_size);
            std::swap(write_size, o.write_size);
            std::swap(read_buffer, o.read_buffer);
            std::swap(transfers, o.transfers);
            std::swap(buffers_pool, o.buffers_pool);
            return *this;
        }
    };


    void construct(implementation_type&);
    void move_construct(implementation_type& impl, implementation_type& other_impl);
    void destroy(implementation_type&);

    void cancel(implementation_type& impl);
    void close(implementation_type& impl);
    bool is_open(implementation_type& impl);
    native_handle_type native_handle(const implementation_type& impl) const;
    tl::expected<void, boost::system::error_code> open(implementation_type& impl);

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
    void set_rts(implementation_type& impl, bool rts);

private:
    friend class usb_device;
    template <typename MutableBufferSequence>
    typename MutableBufferSequence::const_iterator
    read_from_buffer(implementation_type& impl, const typename MutableBufferSequence::const_iterator& begin,
                     const typename MutableBufferSequence::const_iterator& end, std::size_t& read) {
        usb_device_service::buffer& read_buffer = impl.read_buffer;
        for(auto it = begin; it != end; ++it) {
            if(read_buffer.size() < it->size())
                return it;
            read_buffer.read(*it);
            read += it->size();
        }
        return end;
    }


    details::usb_context::ptr m_context;
    bool send_control_transfer(implementation_type& impl);
    bool send_encoding(implementation_type& impl);
};

class usb_device : public boost::asio::basic_io_object<usb_device_service> {
    using ReadDirection = std::true_type;
    using WriteDirection = std::false_type;

public:
    using baud_rate = usb_device_service::baud_rate;
    using data_bits = usb_device_service::data_bits;
    using stop_bits = usb_device_service::stop_bits;
    using flow_control = usb_device_service::flow_control;
    using parity = usb_device_service::parity;

    using native_handle_type = libusb_device_handle*;
    using lowest_layer_type = usb_device;

    usb_device(boost::asio::io_context& io_context);
    ~usb_device();
    usb_device(usb_device&&);
    void assign(libusb_device*);
    void cancel();
    void close();
    bool is_open();
    native_handle_type native_handle() const;
    tl::expected<void, boost::system::error_code> open();

    usb_device_identifier usb_device_id() const;
    std::string usb_device_name() const;
    int usb_device_address() const;

    friend void swap(usb_device& a, usb_device& b);

    template <typename ConstBufferSequence, typename WriteHandler>
    BOOST_ASIO_INITFN_RESULT_TYPE(WriteHandler, void(boost::system::error_code, std::size_t))
    async_write_some(const ConstBufferSequence& buffers, WriteHandler&& handler) {

        static_assert(boost::asio::is_const_buffer_sequence<ConstBufferSequence>::value,
                      "ConstBufferSequence requirements not met");

        boost::asio::async_completion<WriteHandler, void(boost::system::error_code, std::size_t)> init(handler);
        this->async_transfer_some<ConstBufferSequence, WriteHandler, WriteDirection>(
            buffers, std::forward<WriteHandler>(init.completion_handler));
        return init.result.get();
    }

    template <typename MutableBufferSequence, typename ReadHandler>
    BOOST_ASIO_INITFN_RESULT_TYPE(ReadHandler, void(boost::system::error_code, std::size_t))
    async_read_some(const MutableBufferSequence& buffers, ReadHandler&& handler) {

        static_assert(boost::asio::is_const_buffer_sequence<MutableBufferSequence>::value,
                      "MutableBufferSequence requirements not met");

        boost::asio::async_completion<ReadHandler, void(boost::system::error_code, std::size_t)> init(handler);
        this->async_transfer_some<MutableBufferSequence, ReadHandler, ReadDirection>(
            buffers, std::forward<ReadHandler>(init.completion_handler));
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
            return r.value();
        ec = r.error();
        return 0;
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
            return r.value();
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
    void set_rts(bool rts);


    lowest_layer_type& lowest_layer() noexcept {
        return *this;
    }
    const lowest_layer_type& lowest_layer() const noexcept {
        return *this;
    }

private:
    std::queue<std::unique_ptr<libusb_transfer, decltype(&libusb_free_transfer)>> m_transfer_pool;
    libusb_transfer* alloc_transfer() {
        if(m_transfer_pool.empty()) {
            mLogInfo("Allocating an usb tranfer");
            return libusb_alloc_transfer(0);
        }
        auto t = m_transfer_pool.front().release();
        m_transfer_pool.pop();
        return t;
    }
    void free_transfer(libusb_transfer* t) {
        m_transfer_pool.emplace(t, &libusb_free_transfer);
    }


    template <typename BufferSequence, typename CompletionHandler, typename TransferDirection>
    void async_transfer_some(const BufferSequence& buffers, CompletionHandler&& handler);
};

template <typename ConstBufferSequence>
tl::expected<std::size_t, boost::system::error_code>
usb_device_service::write_some(implementation_type& impl, const ConstBufferSequence& buffers) {

    std::size_t total_writen = 0;
    for(const boost::asio::const_buffer& b : boost::beast::buffers_range(buffers)) {
        int written = 0;
        auto err = libusb_bulk_transfer(impl.handle, impl.out_address,
                                        const_cast<unsigned char*>(static_cast<const unsigned char*>(b.data())),
                                        b.size(), &written, 100);
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
    auto it = read_from_buffer<std::remove_reference_t<decltype(buffers)>>(
        impl, boost::asio::buffer_sequence_begin(buffers), boost::asio::buffer_sequence_end(buffers), total_read);
    for(; it != boost::asio::buffer_sequence_end(buffers); ++it) {
        impl.read_buffer.reserve(it->size(), impl.read_size);
        int read = 0;
        auto err = libusb_bulk_transfer(impl.handle, impl.in_address,
                                        static_cast<unsigned char*>(impl.read_buffer.write_begin()),
                                        impl.read_buffer.write_capacity(), &read, 0);
        if(err == LIBUSB_SUCCESS || err == LIBUSB_ERROR_TIMEOUT) {
            total_read += read;
            impl.read_buffer.commit(read);
            impl.read_buffer.read(*it);
            if(err == LIBUSB_ERROR_TIMEOUT)
                break;
        } else {
            return usb::make_unexpected(err);
        }
    }
    return total_read;
}

namespace detail {

    inline usb_device_service::buffer* alloc_buffer(usb_device_service::implementation_type& impl, std::size_t size) {
        auto it = std::find_if(impl.buffers_pool.begin(), impl.buffers_pool.end(),
                               [](const auto& p) { return p.second == true; });
        if(it == impl.buffers_pool.end()) {
            it = impl.buffers_pool.emplace(std::make_shared<usb_device_service::buffer>(), true).first;
        }
        it->second = false;
        usb_device_service::buffer* b = it->first.get();
        b->reserve(size, impl.read_size);
        return b;
    }
    inline void commit_buffer(usb_device_service::implementation_type& impl, uint8_t* head, std::size_t size) {
        auto it = std::find_if(impl.buffers_pool.begin(), impl.buffers_pool.end(),
                               [head](const auto& p) { return p.first->write_begin() == head; });
        assert(it != impl.buffers_pool.end());
        impl.read_buffer.reserve(impl.read_size, size);
        std::copy(head, head + size, impl.read_buffer.write_begin());
        impl.read_buffer.commit(size);
        impl.buffers_pool.erase(it);
    }

    inline void release(usb_device_service::implementation_type& impl, uint8_t* head) {
        auto it = std::find_if(impl.buffers_pool.begin(), impl.buffers_pool.end(),
                               [head](const auto& p) { return p.first->write_begin() == head; });
        assert(it != impl.buffers_pool.end());
        impl.buffers_pool.erase(it);
    }


    template <typename BufferSequence, typename Callback, typename TransferDirection>
    void prepare_transfer(usb_device_service::implementation_type& impl, libusb_transfer* transfer,
                          const BufferSequence& buffers, std::size_t& index, Callback&& cb, void* user_data) {
        uint8_t address;
        uint8_t* buffer;
        const auto it = boost::asio::buffer_sequence_begin(buffers) + index;
        std::size_t buffer_size = 0;
        if constexpr(TransferDirection::value) {
            auto to_read = std::accumulate(it, boost::asio::buffer_sequence_end(buffers), std::size_t{0},
                                           [](const auto& a, const auto& b) { return a + b.size(); });

            auto b = alloc_buffer(impl, to_read);
            buffer_size = b->write_capacity();
            buffer = b->write_begin();
            address = impl.in_address;

        } else {
            buffer_size = it->size();
            buffer = (uint8_t*)(it->data());
            address = impl.out_address;
        }
        libusb_fill_bulk_transfer(transfer, impl.handle, address, buffer, buffer_size, std::forward<Callback>(cb),
                                  user_data, 0);
    }

}  // namespace detail

template <typename BufferSequence, typename CompletionHandler, typename TransferDirection>
void usb_device::async_transfer_some(const BufferSequence& buffers, CompletionHandler&& handler) {

    struct transfer_data {
        BufferSequence seq;
        usb_device& device;
        std::size_t idx;
        std::size_t total_transfered;
        CompletionHandler handler;

        transfer_data(BufferSequence seq, usb_device& device, CompletionHandler&& handler, std::size_t idx = 0,
                      std::size_t total_transfered = 0)
            : seq(seq)
            , device(device)
            , idx(idx)
            , total_transfered(total_transfered)
            , handler(std::forward<CompletionHandler>(handler)) {}
    };

    auto& impl = this->get_implementation();
    auto start_it = boost::asio::buffer_sequence_begin(buffers);
    std::size_t total_transfered = 0;

    if constexpr(TransferDirection::value) {
        start_it = get_service().read_from_buffer<BufferSequence>(
            impl, start_it, boost::asio::buffer_sequence_end(buffers), total_transfered);
        if(start_it == boost::asio::buffer_sequence_end(buffers)) {
            boost::asio::post(get_executor(),
                              boost::beast::bind_handler(std::forward<CompletionHandler>(handler),
                                                         boost::system::error_code{}, total_transfered));
            return;
        }
    }

    static auto cb = [](libusb_transfer* transfer_) {
        auto d = std::unique_ptr<transfer_data>{static_cast<transfer_data*>(transfer_->user_data)};
        const auto deleter = [dev = &d->device](libusb_transfer* t) { dev->free_transfer(t); };
        auto transfer = std::unique_ptr<libusb_transfer, decltype(deleter)>{transfer_, deleter};

        auto& impl = d->device.get_implementation();
        auto it = std::find(impl.transfers.begin(), impl.transfers.end(), transfer_);
        if(it != impl.transfers.end())
            impl.transfers.erase(it);

        // Successful transfer and full buffer => read some more in the next buffer
        if(transfer->status == LIBUSB_TRANSFER_COMPLETED) {
            auto it = boost::asio::buffer_sequence_begin(d->seq) + d->idx;
            if constexpr(TransferDirection::value) {
                detail::commit_buffer(impl, transfer->buffer, transfer->actual_length);
                usb_device_service& service = d->device.get_service();
                it = service.read_from_buffer<BufferSequence>(impl, it, boost::asio::buffer_sequence_end(d->seq),
                                                              d->total_transfered);
                d->idx = std::distance(boost::asio::buffer_sequence_begin(d->seq), it);
            } else {
                d->total_transfered = transfer->actual_length;
                it++;
                d->idx++;
            }


            if(it != boost::asio::buffer_sequence_end(d->seq)) {
                detail::prepare_transfer<BufferSequence, std::remove_reference_t<decltype(transfer->callback)>,
                                         TransferDirection>(impl, transfer.get(), d->seq, d->idx,
                                                            (libusb_transfer_cb_fn)transfer->callback,
                                                            transfer->user_data);
                impl.transfers.push_back(transfer.get());
                libusb_submit_transfer(transfer.release());
                d.release();
                return;
            }
        }
        boost::system::error_code ec;
        if(transfer->status != LIBUSB_TRANSFER_COMPLETED) {
            detail::release(impl, transfer->buffer);
            ec = usb::make_error_code_from_transfer(transfer->status);
        }

        auto handler = std::move(d->handler);
        const auto executor = boost::asio::get_associated_executor(handler, d->device.get_executor());
        const auto total_transfered = d->total_transfered;

        // Reset storage before posting
        d = {};
        transfer = {};

        boost::asio::post(executor,
                          boost::beast::bind_handler(std::forward<CompletionHandler>(handler), ec, total_transfered));
    };


    auto transfer = this->alloc_transfer();
    auto data = std::make_unique<transfer_data>(buffers, *this, std::forward<CompletionHandler>(handler),
                                                std::distance(boost::asio::buffer_sequence_begin(buffers), start_it),
                                                total_transfered);
    detail::prepare_transfer<BufferSequence, std::remove_reference_t<decltype(cb)>, TransferDirection>(
        impl, transfer, data->seq, data->idx, std::move(cb), data.get());
    auto r = libusb_submit_transfer(transfer);
    if(r != LIBUSB_SUCCESS) {
        this->free_transfer(transfer);
        auto handler = std::move(data->handler);
        boost::asio::post(
            data->device.get_executor(),
            boost::beast::bind_handler(std::forward<CompletionHandler>(handler), usb::make_error_code(r), 0));
    } else {
        impl.transfers.push_back(transfer);
        data.release();
    }
}

}  // namespace mobsya
