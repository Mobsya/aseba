#pragma once
#include <functional>
#include <memory>
#include <unordered_map>
#include <boost/asio.hpp>
#include <chrono>
#include "usb_utils.h"
#ifdef MOBSYA_TDM_ENABLE_USB
#	include "usbdevice.h"
#endif
#ifdef MOBSYA_TDM_ENABLE_SERIAL
#	include "serial_usb_device.h"
#endif
#include "aseba_message_parser.h"
#include "aseba_message_writer.h"
#include "aseba_description_receiver.h"
#include "aseba_node.h"
#include "log.h"
//#include "variant.hpp"
#include "aseba_node_registery.h"

namespace mobsya {

constexpr usb_device_identifier THYMIO2_DEVICE_ID = {0x0617, 0x000a};
constexpr usb_device_identifier THYMIO_WIRELESS_DEVICE_ID = {0x0617, 0x000c};

namespace variant_ns = std;

class aseba_endpoint : public std::enable_shared_from_this<aseba_endpoint> {


public:
    enum class endpoint_type { unknown, thymio, simulated_tymio, simulated_epuck, simulated_dummy_node, legacy_switch };

    ~aseba_endpoint() {
        std::for_each(std::begin(m_nodes), std::end(m_nodes), [](auto&& node) { node.second.node->disconnect(); });
    }

    using pointer = std::shared_ptr<aseba_endpoint>;
    using tcp_socket = boost::asio::ip::tcp::socket;
    using endpoint_t = variant_ns::variant<tcp_socket
#ifdef MOBSYA_TDM_ENABLE_USB
		, usb_device
#endif
#ifdef MOBSYA_TDM_ENABLE_SERIAL
		, usb_serial_port
#endif
	>;

    using write_callback = std::function<void(boost::system::error_code)>;

#ifdef MOBSYA_TDM_ENABLE_USB
    const usb_device& usb() const {
        return variant_ns::get<usb_device>(m_endpoint);
    }

    usb_device& usb() {
        return variant_ns::get<usb_device>(m_endpoint);
    }

	static pointer create_for_usb(boost::asio::io_context& io) {
		return std::shared_ptr<aseba_endpoint>(new aseba_endpoint(io, usb_device(io)));
	}
#endif

#ifdef MOBSYA_TDM_ENABLE_SERIAL
	const usb_serial_port& serial() const {
		return variant_ns::get<usb_serial_port>(m_endpoint);
	}

	usb_serial_port& serial() {
		return variant_ns::get<usb_serial_port>(m_endpoint);
	}

	static pointer create_for_serial(boost::asio::io_context& io) {
		return std::shared_ptr<aseba_endpoint>(new aseba_endpoint(io, usb_serial_port(io)));
	}
#endif

	const tcp_socket& tcp() const {
        return variant_ns::get<tcp_socket>(m_endpoint);
    }

    tcp_socket& tcp() {
        return variant_ns::get<tcp_socket>(m_endpoint);
    }

    static pointer create_for_tcp(boost::asio::io_context& io) {
        return std::shared_ptr<aseba_endpoint>(new aseba_endpoint(io, tcp_socket(io)));
    }

    std::string endpoint_name() const {
        return m_endpoint_name;
    }
    void set_endpoint_name(const std::string& name) {
        m_endpoint_name = name;
    }

    endpoint_type type() const {
        return m_endpoint_type;
    }
    void set_endpoint_type(endpoint_type type) {
        m_endpoint_type = type;
    }

    void start() {
        read_aseba_message();


        // A newly connected thymio may not be ready yet
        // Delay asking for its node id to let it start up the vm
        // otherwhise it may never get our request.
        schedule_send_ping(boost::posix_time::milliseconds(200));

        if(needs_health_check())
            schedule_nodes_health_check();
    }

    template <typename CB = write_callback>
    void write_messages(std::vector<std::shared_ptr<Aseba::Message>>&& messages, CB&& cb = {}) {
        if(messages.empty())
            return;
        std::unique_lock<std::mutex> _(m_msg_queue_lock);
        for(auto&& m : messages) {
            m_msg_queue.emplace_back(std::move(m), write_callback{});
        }
        if(cb) {
            m_msg_queue.back().second = std::move(cb);
        }
        if(m_msg_queue.size() > messages.size())
            return;
        do_write_message(*(m_msg_queue.front().first));
    }

    template <typename CB = write_callback>
    void write_message(std::shared_ptr<Aseba::Message> message, CB&& cb = {}) {
        write_messages({std::move(message)}, std::forward<CB>(cb));
    }

    void read_aseba_message() {
        auto that = shared_from_this();
        auto cb = boost::asio::bind_executor(
            m_strand,
            [that](boost::system::error_code ec, std::shared_ptr<Aseba::Message> msg) { that->handle_read(ec, msg); });

		variant_ns::visit([&cb](auto & underlying) {
			return mobsya::async_read_aseba_message(underlying, std::move(cb));
		}, m_endpoint);
    }

    void handle_read(boost::system::error_code ec, std::shared_ptr<Aseba::Message> msg) {
        if(ec) {
            mLogError("Error while reading aseba message {}", ec.message());
            return;
        }
        mLogTrace("Message received : {} {}", ec.message(), msg->type);

        auto node_id = msg->source;
        auto it = m_nodes.find(node_id);
        auto node = it == std::end(m_nodes) ? std::shared_ptr<aseba_node>{} : it->second.node;
        if(msg->type == ASEBA_MESSAGE_NODE_PRESENT) {
            if(!node) {
                m_nodes.insert({node_id,
                                {aseba_node::create(m_io_context, node_id, shared_from_this()),
                                 std::chrono::steady_clock::now()}});
                // Reading move this, we need to return immediately after
                read_aseba_node_description(node_id);
                return;
            }
            // Update node status
            it->second.last_seen = std::chrono::steady_clock::now();

        } else if(node) {
            node->on_message(*msg);
        }
        read_aseba_message();
    }

private:
    friend class aseba_node;

    struct node_info {
        std::shared_ptr<aseba_node> node;
        std::chrono::steady_clock::time_point last_seen;
    };

    std::shared_ptr<aseba_node> find_node(const Aseba::Message& message) {
        return find_node(message.source);
    }

    std::shared_ptr<aseba_node> find_node(uint16_t node) {
        auto it = m_nodes.find(node);
        if(it == m_nodes.end()) {
            return {};
        }
        return it->second.node;
    }

    void schedule_send_ping(boost::posix_time::time_duration delay = boost::posix_time::seconds(1)) {
        auto timer = std::make_shared<boost::asio::deadline_timer>(m_io_context);
        timer->expires_from_now(delay);
        std::weak_ptr<aseba_endpoint> ptr = this->shared_from_this();
        auto cb = boost::asio::bind_executor(m_strand, [timer, ptr](const boost::system::error_code& ec) {
            auto that = ptr.lock();
            if(!that || ec)
                return;
            mLogInfo("Requesting list nodes( ec : {} )", ec.message());
            that->write_message(std::make_unique<Aseba::ListNodes>());
            if(that->needs_health_check())
                that->schedule_send_ping();
        });
        mLogDebug("Waiting before requesting list node");
        timer->async_wait(std::move(cb));
    }

    void schedule_nodes_health_check(boost::posix_time::time_duration delay = boost::posix_time::seconds(1)) {
        auto timer = std::make_shared<boost::asio::deadline_timer>(m_io_context);
        timer->expires_from_now(delay);
        std::weak_ptr<aseba_endpoint> ptr = this->shared_from_this();
        auto cb = boost::asio::bind_executor(m_strand, [this, timer, ptr](const boost::system::error_code&) {
            auto that = ptr.lock();
            if(!that)
                return;
            auto now = std::chrono::steady_clock::now();
            for(auto it = m_nodes.begin(); it != m_nodes.end();) {
                const auto& info = it->second;
                auto d = std::chrono::duration_cast<std::chrono::seconds>(now - info.last_seen);
                if(d.count() >= 3) {
                    mLogTrace("Node {} has been unresponsive for too long, disconnecting it!",
                              it->second.node->native_id());
                    info.node->set_status(aseba_node::status::disconnected);
                    it = m_nodes.erase(it);
                } else {
                    ++it;
                }
            }
            // Reschedule
            that->schedule_nodes_health_check();
        });
        timer->async_wait(std::move(cb));
    }

    // Do not run pings / health check for usb-connected nodes
    bool needs_health_check() const {
#ifdef MOBSYA_TDM_ENABLE_USB
        return variant_ns::holds_alternative<usb_device>(m_endpoint) &&
            usb().usb_device_id() == THYMIO_WIRELESS_DEVICE_ID;
#endif
		return false;
    }

    void read_aseba_node_description(uint16_t node) {
        auto that = shared_from_this();
        auto cb = boost::asio::bind_executor(
            m_strand, [that](boost::system::error_code ec, uint16_t id, Aseba::TargetDescription msg) {
                if(ec) {
                    mLogError("Error while waiting for a node description");
                    return;
                }
                auto node = that->find_node(id);
                that->read_aseba_message();
                node->on_description(msg);
            });

        mLogInfo("Asking for description of node {}", node);
        write_message(std::make_unique<Aseba::GetNodeDescription>(node));

		variant_ns::visit([&cb, &node](auto & underlying) {
			return mobsya::async_read_aseba_description_message(underlying, node, std::move(cb));
		}, m_endpoint);
    }

    void do_write_message(const Aseba::Message& message) {
        auto that = shared_from_this();
        auto cb =
            boost::asio::bind_executor(m_strand, [that](boost::system::error_code ec) { that->handle_write(ec); });
		
		variant_ns::visit([&cb, &message](auto & underlying) {
			return mobsya::async_write_aseba_message(underlying, message, std::move(cb));
		}, m_endpoint);
    }

    void handle_write(boost::system::error_code ec) {
        mLogDebug("Message sent : {}", ec.message());
        std::unique_lock<std::mutex> _(m_msg_queue_lock);
        auto cb = m_msg_queue.begin()->second;
        if(cb) {
            boost::asio::post(m_io_context.get_executor(), std::bind(std::move(cb), ec));
        }
        m_msg_queue.erase(m_msg_queue.begin());
        if(!m_msg_queue.empty()) {
            do_write_message(*(m_msg_queue.front().first));
        }
    }

    aseba_endpoint(boost::asio::io_context& io_context, endpoint_t&& e, endpoint_type type = endpoint_type::thymio)
        : m_endpoint(std::move(e))
        , m_strand(io_context.get_executor())
        , m_io_context(io_context)
        , m_endpoint_type(type) {}
    endpoint_t m_endpoint;
    boost::asio::strand<boost::asio::io_context::executor_type> m_strand;
    std::unordered_map<aseba_node::node_id_t, node_info> m_nodes;
    boost::asio::io_service& m_io_context;
    endpoint_type m_endpoint_type;
    std::string m_endpoint_name;
    std::mutex m_msg_queue_lock;
    std::vector<std::pair<std::shared_ptr<Aseba::Message>, write_callback>> m_msg_queue;
};

}  // namespace mobsya
