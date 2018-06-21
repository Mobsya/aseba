#pragma once
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <aseba/flatbuffers/thymio_generated.h>
#include "log.h"

namespace mobsya {

template <class AsyncReadStream, class Handler>
class read_flatbuffers_message_op;


class fb_message_ptr {
public:
    fb_message_ptr() : m_msg(nullptr) {}
    fb_message_ptr(const fb_message_ptr&) = delete;
    fb_message_ptr(fb_message_ptr&& other) : m_data(std::move(other.m_data)), m_msg(other.m_msg) {
        other.m_msg = nullptr;
    }
    fb_message_ptr(std::vector<uint8_t>&& data) : m_data(std::move(data)) {
        m_msg = fb::GetMessage(m_data.data());
    }

    operator bool() const {
        return m_msg;
    }

    const fb::Message* operator->() const {
        return m_msg;
    }

    const fb::Message& operator*() const {
        return *m_msg;
    }

    fb::AnyMessage message_type() const {
        return m_msg->message_type();
    }

    template <typename T>
    const T* as() const {
        return m_msg->message_as<T>();
    }

private:
    std::vector<uint8_t> m_data;
    const fb::Message* m_msg;
};

using read_flatbuffers_message_op_cb_t = void(boost::system::error_code, fb_message_ptr);

template <class AsyncReadStream, class CompletionToken>
BOOST_ASIO_INITFN_RESULT_TYPE(CompletionToken, read_flatbuffers_message_op_cb_t)
async_read_flatbuffers_message(AsyncReadStream& stream, CompletionToken&& token) {
    static_assert(boost::beast::is_async_read_stream<AsyncReadStream>::value, "AsyncReadStream requirements not met");

    boost::asio::async_completion<CompletionToken, read_flatbuffers_message_op_cb_t> init{token};
    read_flatbuffers_message_op<AsyncReadStream,
                                BOOST_ASIO_HANDLER_TYPE(CompletionToken, read_flatbuffers_message_op_cb_t)>{
        stream, std::forward<CompletionToken>(init.completion_handler)}();

    return init.result.get();
}


template <class AsyncReadStream, class Handler>
class read_flatbuffers_message_op {
    struct state {
        AsyncReadStream& stream;
        char sizeBuffer[2];
        uint8_t size = 0;
        std::vector<uint8_t> dataBuffer;

        explicit state(Handler const& handler, AsyncReadStream& stream) : stream(stream) {}
    };
    boost::beast::handler_ptr<state, Handler> m_p;

public:
    read_flatbuffers_message_op(read_flatbuffers_message_op&&) = default;
    read_flatbuffers_message_op(read_flatbuffers_message_op const&) = default;

    template <class DeducedHandler, class... Args>
    read_flatbuffers_message_op(AsyncReadStream& stream, DeducedHandler&& handler)
        : m_p(std::forward<DeducedHandler>(handler), stream) {}

    using allocator_type = boost::asio::associated_allocator_t<Handler>;

    allocator_type get_allocator() const noexcept {
        return boost::asio::get_associated_allocator(m_p.handler());
    }

    using executor_type =
        boost::asio::associated_executor_t<Handler, decltype(std::declval<AsyncReadStream&>().get_executor())>;

    executor_type get_executor() const noexcept {
        return boost::asio::get_associated_executor(m_p.handler(), m_p->stream.get_executor());
    }

    void operator()() {
        auto& state = *m_p;
        return boost::asio::async_read(state.stream, boost::asio::buffer(state.sizeBuffer),
                                       boost::asio::transfer_exactly(2), std::move(*this));
    }

    void operator()(boost::system::error_code ec, std::size_t bytes_transferred) {
        mLogError("{}", ec.message());
        auto& state = *m_p;
        if(bytes_transferred == 0)
            return;
        if(state.dataBuffer.size() == 0) {
            assert(bytes_transferred == 2);
            auto size = reinterpret_cast<uint16_t&>(state.sizeBuffer);
            state.dataBuffer.resize(size);
            return boost::asio::async_read(state.stream, boost::asio::buffer(state.dataBuffer),
                                           boost::asio::transfer_exactly(size), std::move(*this));
        }
        if(bytes_transferred == state.dataBuffer.size()) {
            auto& b = state.dataBuffer;
            m_p.invoke(ec, fb_message_ptr(std::move(b)));
            return;
        }
        m_p.invoke(ec, std::move(fb_message_ptr{}));
    }
};
}  // namespace mobsya
