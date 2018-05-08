#pragma once
#include <memory>
#include <boost/asio.hpp>
#include "usbdevice.h"
#include "aseba_message_parser.h"
#include "aseba_message_writer.h"
#include "log.h"
#include "variant.hpp"

namespace mobsya {

namespace variant_ns = nonstd;

class aseba_endpoint : public std::enable_shared_from_this<aseba_endpoint> {


public:
    using pointer = std::shared_ptr<aseba_endpoint>;
    using endpoint_t = variant_ns::variant<usb_device>;

    usb_device& usb() {
        return variant_ns::get<usb_device>(m_endpoint);
    }

    usb_device& socket() {
        return variant_ns::get<usb_device>(m_endpoint);
    }


    static pointer create_for_usb(boost::asio::io_context& io) {
        return std::shared_ptr<aseba_endpoint>(new aseba_endpoint(io, usb_device(io)));
    }
    static pointer create_for_tcp(boost::asio::io_context& io) {
        //   return std::shared_ptr<aseba_endpoint>(new aseba_endpoint(io, endpoint_type::tcp));
    }


    void start() {
        read_aseba_message();
        write_aseba_message(Aseba::ListNodes());
    }


    void write_aseba_message(const Aseba::Message& message) {
        auto that = shared_from_this();
        auto cb = boost::asio::bind_executor(
            m_strand, std::move([that](boost::system::error_code ec) { that->handle_write(ec); }));
        if(variant_ns::holds_alternative<usb_device>(m_endpoint)) {
            mobsya::async_write_aseba_message(variant_ns::get<usb_device>(m_endpoint), message, std::move(cb));
        } else if(variant_ns::holds_alternative<boost::asio::ip::tcp::socket>(m_endpoint)) {
            mobsya::async_write_aseba_message(variant_ns::get<boost::asio::ip::tcp::socket>(m_endpoint), message,
                                              std::move(cb));
        }
    }

    void read_aseba_message() {
        auto that = shared_from_this();
        auto cb = boost::asio::bind_executor(
            m_strand, std::move([that](boost::system::error_code ec, std::shared_ptr<Aseba::Message> msg) {
                that->handle_read(ec, msg);
            }));

        if(variant_ns::holds_alternative<usb_device>(m_endpoint)) {
            mobsya::async_read_aseba_message(variant_ns::get<usb_device>(m_endpoint), std::move(cb));
        } else if(variant_ns::holds_alternative<boost::asio::ip::tcp::socket>(m_endpoint)) {
            mobsya::async_read_aseba_message(variant_ns::get<boost::asio::ip::tcp::socket>(m_endpoint), std::move(cb));
        }
    }

    void handle_read(boost::system::error_code ec, std::shared_ptr<Aseba::Message> msg) {
        mLogInfo("Message received : {} {}", ec.message(), msg->type);
        msg->dump(std::wcout);
    }
    void handle_write(boost::system::error_code ec) {
        mLogInfo("Message sent : {}", ec.message());
    }

private:
    enum class endpoint_type { usb, tcp };
    aseba_endpoint(boost::asio::io_service& io_context, endpoint_t&& e)
        : m_endpoint(std::move(e)), m_strand(io_context.get_executor()) {}
    endpoint_t m_endpoint;
    boost::asio::strand<boost::asio::io_context::executor_type> m_strand;
};

}  // namespace mobsya
