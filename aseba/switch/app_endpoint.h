#pragma once
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <memory>
#include <type_traits>
#include "flatbuffers_message_writer.h"
#include "flatbuffers_message_reader.h"
#include <aseba/flatbuffers/thymio_generated.h>
#include "aseba_node_registery.h"

#include "log.h"

namespace mobsya {
using tcp = boost::asio::ip::tcp;
namespace websocket = boost::beast::websocket;
using websocket_t = websocket::stream<tcp::socket>;

template <typename Self, typename Socket>
class application_endpoint_base : public std::enable_shared_from_this<application_endpoint_base<Self, Socket>> {
public:
    application_endpoint_base(boost::asio::io_context& ctx) = delete;
    void read_message() = delete;
    void start() = delete;
    void write_message(flatbuffers::DetachedBuffer&& buffer) = delete;
    tcp::socket& tcp_socket() = delete;
};


template <typename Self>
class application_endpoint_base<Self, websocket_t>
    : public std::enable_shared_from_this<application_endpoint_base<Self, websocket_t>> {
public:
    application_endpoint_base(boost::asio::io_context& ctx)
        : m_ctx(ctx), m_socket(tcp::socket(ctx)), m_strand(ctx.get_executor()) {}
    void read_message() {
        auto that = this->shared_from_this();


        auto cb = boost::asio::bind_executor(
            m_strand, std::move([this, that](boost::system::error_code ec, std::size_t bytes_transferred) mutable {
                if(ec) {
                    mLogError("read_message :{}", ec.message());
                    return;
                }
                std::vector<uint8_t> buf(boost::asio::buffers_begin(this->m_buffer.data()),
                                         boost::asio::buffers_begin(this->m_buffer.data()) + bytes_transferred);
                fb_message_ptr msg(std::move(buf));
                static_cast<Self&>(*that).handle_read(ec, std::move(msg));
            }));
        m_socket.async_read(m_buffer, std::move(cb));
    }

    void write_message(flatbuffers::DetachedBuffer&& buffer) {
        auto that = this->shared_from_this();
        boost::asio::const_buffer netbuf(buffer.data(), buffer.size());
        auto cb = boost::asio::bind_executor(
            m_strand, std::move([buffer = std::move(buffer), that](boost::system::error_code ec, std::size_t s) {
                static_cast<Self*>(that)->handle_write(ec);
            }));
        m_socket.async_write(netbuf, std::move(cb));
    }
    void start() {
        auto that = this->shared_from_this();
        auto cb = boost::asio::bind_executor(m_strand, std::move([that](boost::system::error_code ec) mutable {
                                                 static_cast<Self&>(*that).on_initialized(ec);
                                             }));
        m_socket.async_accept(std::move(cb));
    }

    tcp::socket& tcp_socket() {
        return m_socket.next_layer();
    }

protected:
    boost::asio::io_context& m_ctx;

private:
    boost::beast::multi_buffer m_buffer;
    websocket_t m_socket;
    boost::asio::strand<boost::asio::io_context::executor_type> m_strand;
};


template <typename Self>
class application_endpoint_base<Self, tcp::socket>
    : public std::enable_shared_from_this<application_endpoint_base<Self, tcp::socket>> {
public:
    application_endpoint_base(boost::asio::io_context& ctx)
        : m_ctx(ctx), m_socket(tcp::socket(ctx)), m_strand(ctx.get_executor()) {}
    void read_message() {
        auto that = this->shared_from_this();
        auto cb =
            boost::asio::bind_executor(m_strand, std::move([that](boost::system::error_code ec, fb_message_ptr msg) {
                                           static_cast<Self*>(that)->handle_read(ec, std::move(msg));
                                       }));
        mobsya::async_read_flatbuffers_message(m_socket, std::move(cb));
    }

    void write_message(flatbuffers::DetachedBuffer&& buffer) {
        auto that = this->shared_from_this();
        auto cb = boost::asio::bind_executor(
            m_strand, std::move([buffer = std::move(buffer), that](boost::system::error_code ec, std::size_t s) {
                static_cast<Self*>(that)->handle_write(ec);
            }));
        mobsya::async_write_flatbuffer_message(m_socket, std::move(buffer), std::move(cb));
    }

    void start() {
        static_cast<Self*>(this)->on_initialized();
    }

    tcp::socket& tcp_socket() {
        return m_socket;
    }

protected:
    boost::asio::io_context& m_ctx;

private:
    tcp::socket m_socket;
    boost::asio::strand<boost::asio::io_context::executor_type> m_strand;
};

template <typename Socket>
class application_endpoint : public application_endpoint_base<application_endpoint<Socket>, Socket>,
                             public node_status_monitor {
public:
    using base = application_endpoint_base<application_endpoint<Socket>, Socket>;
    application_endpoint(boost::asio::io_context& ctx) : base(ctx) {}
    void start() {
        mLogInfo("Starting app endpoint");
        base::start();
    }

    void on_initialized(boost::system::error_code ec = {}) {
        mLogError("on_initialized: {}", ec.message());

        // start listening for incomming messages
        read_message();

        // Subscribe to node change events
        start_node_monitoring(registery());

        // Immediately send a list of nodes
        send_full_node_list();
    }

    void read_message() {
        base::read_message();
    }

    void write_message(flatbuffers::DetachedBuffer&& buffer) {
        base::write_message(std::move(buffer));
    }


    void handle_read(boost::system::error_code ec, fb_message_ptr&& msg) {
        mLogError("{} -> {} ", ec.message(), EnumNameAnyMessage(msg.message_type()));
        read_message();
    }

    void handle_write(boost::system::error_code ec) {
        mLogError("{}", ec.message());
    }

    ~application_endpoint() {
        mLogInfo("Stopping app endpoint");
    }

    void node_changed(std::shared_ptr<aseba_node> node, aseba_node_registery::node_id, aseba_node::status status) {
        mLogInfo("node changed: {}, {}", node->native_id(), node->status_to_string(status));
    }

private:
    void send_full_node_list() {
        auto& r = registery();
    }

    aseba_node_registery& registery() {
        return boost::asio::use_service<aseba_node_registery>(this->m_ctx);
    }
};


}  // namespace mobsya
