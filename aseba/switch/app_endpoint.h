#pragma once
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <memory>
#include "variant.hpp"
#include "flatbuffers_message_writer.h"
#include "flatbuffers_message_reader.h"

namespace mobsya {
using tcp = boost::asio::ip::tcp;
namespace websocket = boost::beast::websocket;
namespace variant_ns = nonstd;

class application_endpoint : public std::enable_shared_from_this<application_endpoint> {
public:
    using tcp_socket_t = tcp::socket;
    using websocket_t = websocket::stream<tcp::socket>;
    using pointer = std::shared_ptr<application_endpoint>;
    using socket = variant_ns::variant<tcp_socket_t, websocket_t>;


    static pointer create_tcp_endpoint(boost::asio::io_context& ctx) {
        return std::make_shared<application_endpoint>(ctx);
    }

    struct web_socket_tag {};
    static pointer create_websocket_endpoint(boost::asio::io_context& ctx) {
        return std::make_shared<application_endpoint>(ctx, web_socket_tag{});
    }

    tcp::socket& tcp_socket() {
        if(variant_ns::holds_alternative<tcp_socket_t>(m_socket))
            return variant_ns::get<tcp_socket_t>(m_socket);
        else
            return variant_ns::get<websocket_t>(m_socket).next_layer();
    }

    application_endpoint(boost::asio::io_context& ctx) : m_ctx(ctx), m_socket(tcp_socket_t(ctx)) {}

    application_endpoint(boost::asio::io_context& ctx, web_socket_tag) : m_ctx(ctx), m_socket(websocket_t(ctx)) {}


    void start() {}


private:
    boost::asio::io_context& m_ctx;
    socket m_socket;
    std::optional<websocket_t> m_ws_stream;
};

template <typename T>
application_endpoint::pointer create_application_endpoint(boost::asio::io_context& ctx) = delete;

template <>
application_endpoint::pointer
create_application_endpoint<application_endpoint::tcp_socket_t>(boost::asio::io_context& ctx) {
    return application_endpoint::create_tcp_endpoint(ctx);
}

template <>
application_endpoint::pointer
create_application_endpoint<application_endpoint::websocket_t>(boost::asio::io_context& ctx) {
    return application_endpoint::create_websocket_endpoint(ctx);
}


}  // namespace mobsya
