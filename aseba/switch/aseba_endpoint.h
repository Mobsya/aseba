#pragma once
#include <memory>
#include <unordered_map>
#include <boost/asio.hpp>
#include "usbdevice.h"
#include "aseba_message_parser.h"
#include "aseba_message_writer.h"
#include "aseba_description_receiver.h"
#include "aseba_node.h"
#include "log.h"
#include "variant.hpp"
#include "aseba_node_registery.h"

namespace mobsya {

namespace variant_ns = nonstd;

class aseba_endpoint : public std::enable_shared_from_this<aseba_endpoint> {


public:
    ~aseba_endpoint() {
        std::for_each(std::begin(m_nodes), std::end(m_nodes), [](auto&& node) { node.second->disconnect(); });
    }

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


        // A newly connected thymio may not be ready yet
        // Delay asking for its node id to let it start up the vm
        // otherwhise it may never get our request.

        auto timer = std::make_shared<boost::asio::deadline_timer>(m_io_context);
        timer->expires_from_now(boost::posix_time::milliseconds(200));
        auto that = shared_from_this();
        auto cb = boost::asio::bind_executor(m_strand, std::move([timer, that](const boost::system::error_code& ec) {
                                                 mLogInfo("Requesting list nodes( ec : {}", ec.message());
                                                 that->write_message(std::make_unique<Aseba::ListNodes>());
                                             }));
        mLogInfo("Waiting before requesting list node");
        timer->async_wait(std::move(cb));
    }

    void write_messages(std::vector<std::shared_ptr<Aseba::Message>>&& messages) {
        std::unique_lock<std::mutex> _(m_msg_queue_lock);
        for(auto&& m : messages) {
            m_msg_queue.push_back(std::move(m));
        }
        if(m_msg_queue.size() > messages.size())
            return;
        do_write_message(*m_msg_queue.front());
    }

    void write_message(std::shared_ptr<Aseba::Message> message) {
        write_messages({std::move(message)});
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

    void broadcast(std::shared_ptr<Aseba::Message> msg) {
        auto& registery = boost::asio::use_service<aseba_node_registery>(m_io_context);
        registery.broadcast(msg);
    }

    void handle_read(boost::system::error_code ec, std::shared_ptr<Aseba::Message> msg) {
        mLogInfo("Message received : {} {}", ec.message(), msg->type);
        msg->dump(std::wcout);
        std::wcout << std::endl;

        if(msg->type == ASEBA_MESSAGE_NODE_PRESENT) {
            auto node_id = msg->source;
            auto it = m_nodes.find(node_id);
            const auto new_node = it == m_nodes.end();
            if(new_node) {
                it = m_nodes.insert({node_id, aseba_node::create(m_io_context, node_id, shared_from_this())}).first;
                // Reading move this, we need to return immediately after
                read_aseba_node_description(node_id);
                return;
            }
        }
        auto node = find_node(*msg);
        if(node) {
            node->on_message(*msg);
        }
        read_aseba_message();
    }

private:
    friend class aseba_node;

    std::shared_ptr<aseba_node> find_node(const Aseba::Message& message) {
        return find_node(message.source);
    }
    std::shared_ptr<aseba_node> find_node(uint16_t node) {
        auto it = m_nodes.find(node);
        if(it == m_nodes.end()) {
            return {};
        }
        return it->second;
    }

    void read_aseba_node_description(uint16_t node) {
        auto that = shared_from_this();
        auto cb = boost::asio::bind_executor(
            m_strand, std::move([that](boost::system::error_code ec, uint16_t id, Aseba::TargetDescription msg) {
                if(ec) {
                    mLogError("Error while waiting for a node description");
                }
                auto node = that->find_node(id);
                node->on_description(msg);
                that->read_aseba_message();
            }));

        mLogInfo("Asking for description of node {}", node);
        write_message(std::make_unique<Aseba::GetNodeDescription>(node));

        if(variant_ns::holds_alternative<usb_device>(m_endpoint)) {
            mobsya::async_read_aseba_description_message(variant_ns::get<usb_device>(m_endpoint), node, std::move(cb));
        } else if(variant_ns::holds_alternative<boost::asio::ip::tcp::socket>(m_endpoint)) {
            mobsya::async_read_aseba_description_message(variant_ns::get<boost::asio::ip::tcp::socket>(m_endpoint),
                                                         node, std::move(cb));
        }
    }

    void do_write_message(const Aseba::Message& message) {
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

    void handle_write(boost::system::error_code ec) {
        mLogInfo("Message sent : {}", ec.message());
        std::unique_lock<std::mutex> _(m_msg_queue_lock);
        m_msg_queue.erase(m_msg_queue.begin());
        if(!m_msg_queue.empty()) {
            do_write_message(*m_msg_queue.front());
        }
    }


    enum class endpoint_type { usb, tcp };
    aseba_endpoint(boost::asio::io_context& io_context, endpoint_t&& e)
        : m_endpoint(std::move(e)), m_strand(io_context.get_executor()), m_io_context(io_context) {}
    endpoint_t m_endpoint;
    boost::asio::strand<boost::asio::io_context::executor_type> m_strand;
    std::unordered_map<aseba_node::node_id_t, std::shared_ptr<aseba_node>> m_nodes;
    boost::asio::io_service& m_io_context;
    std::mutex m_msg_queue_lock;
    std::vector<std::shared_ptr<Aseba::Message>> m_msg_queue;
};

}  // namespace mobsya
