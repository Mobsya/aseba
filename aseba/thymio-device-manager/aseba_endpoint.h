#pragma once
#include <functional>
#include <memory>
#include <unordered_map>
#include <queue>
#include <boost/asio.hpp>
#include <chrono>
#include "aseba_message_parser.h"
#include "aseba_message_writer.h"
#include "aseba_node.h"
#include "log.h"
#include "group.h"
#include "variant_compat.h"
#include "utils.h"
#include "thymio2_fwupgrade.h"
#include "uuid_provider.h"
#include "aseba_device.h"

namespace mobsya {

class aseba_endpoint : public std::enable_shared_from_this<aseba_endpoint> {


public:
    enum class endpoint_type { unknown, thymio, simulated_thymio, simulated_dummy_node };

    ~aseba_endpoint();
    void destroy();

    using pointer = std::shared_ptr<aseba_endpoint>;
    using tcp_socket = boost::asio::ip::tcp::socket;

    using write_callback = std::function<void(boost::system::error_code)>;

    void set_group(std::shared_ptr<mobsya::group> g) {
        m_group = g;
    }

    std::shared_ptr<mobsya::group> group() const {
        return m_group;
    }

#ifdef MOBSYA_TDM_ENABLE_USB
    const usb_device& usb() const {
        return m_endpoint.usb();
    }

    usb_device& usb() {
        return m_endpoint.usb();
    }

    static pointer create_for_usb(boost::asio::io_context& io) {
        auto ptr = std::shared_ptr<aseba_endpoint>(new aseba_endpoint(io, aseba_device(usb_device(io))));
        ptr->m_group = group::make_group_for_endpoint(io, ptr);
        return ptr;
    }
#endif

#ifdef MOBSYA_TDM_ENABLE_SERIAL
    const usb_serial_port& serial() const {
        return m_endpoint.serial();
    }

    usb_serial_port& serial() {
        return m_endpoint.serial();
    }

    static pointer create_for_serial(boost::asio::io_context& io);
#endif

#ifdef MOBSYA_TDM_ENABLE_TCP
    const tcp_socket& tcp() const {
        return m_endpoint.tcp();
    }

    tcp_socket& tcp() {
        return m_endpoint.tcp();
    }
#endif

    static pointer create_for_tcp(boost::asio::io_context& io);

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

    node_id uuid() {
        return m_uuid;
    }

    bool is_usb() const {
        return m_endpoint.is_usb();
    }

    bool is_wireless() const {
        return m_endpoint.is_wireless();
    }


#ifdef HAS_FIRMWARE_UPDATE
    bool upgrade_firmware(uint16_t id, std::function<void(boost::system::error_code, double, bool)> cb,
                          firmware_update_options options = firmware_update_options::no_option);
#endif

    std::vector<std::shared_ptr<aseba_node>> nodes() const {
        std::vector<std::shared_ptr<aseba_node>> nodes;
        std::transform(m_nodes.begin(), m_nodes.end(), std::back_inserter(nodes),
                       [](auto&& pair) { return pair.second.node; });
        return nodes;
    }

    void start();
    void close();
    void stop();
    void cancel_all_ops();

    template <typename CB = write_callback>
    void write_messages(std::vector<std::shared_ptr<Aseba::Message>>&& messages, CB&& cb = {}) {
        if(messages.empty())
            return;
        std::unique_lock<std::mutex> _(m_msg_queue_lock);

        auto it = m_msg_queue.end();
        for(; it != m_msg_queue.begin(); --it) {
            if(it == m_msg_queue.end())
                break;
            if(it->first->type != ASEBA_MESSAGE_LIST_NODES && it->first->type != ASEBA_MESSAGE_GET_EXECUTION_STATE &&
               it->first->type != ASEBA_MESSAGE_GET_CHANGED_VARIABLES)
                break;
        }
        for(auto&& m : messages) {
            it = m_msg_queue.insert(it, {std::move(m), write_callback{}});
        }
        if(cb) {
            it->second = std::move(cb);
        }
        if(m_msg_queue.size() > messages.size())
            return;
        write_next();
    }

    template <typename CB = write_callback>
    void write_message(std::shared_ptr<Aseba::Message> message, CB&& cb = {}) {
        write_messages({std::move(message)}, std::forward<CB>(cb));
    }

    void read_aseba_message();
    void handle_read(boost::system::error_code ec, std::shared_ptr<Aseba::Message> msg);
    void remove_node(node_id n);

    const aseba_device* device() const {
        return &m_endpoint;
    }
    aseba_device* device() {
        return &m_endpoint;
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

    void schedule_send_ping(boost::posix_time::time_duration delay = boost::posix_time::seconds(1));
    void schedule_nodes_health_check(boost::posix_time::time_duration delay = boost::posix_time::seconds(5));


    // Do not run pings / health check for usb-connected nodes
    bool needs_health_check() const {
        if(m_endpoint.is_tcp() || m_endpoint.is_wireless()) {
            return true;
        }
        return false;
    }

    bool needs_ping() const {
        if(m_endpoint.is_usb())
            return true;
        return needs_health_check();
    }

    void handle_write(boost::system::error_code ec) {
        if(m_msg_queue.empty())
            return;
        std::unique_lock<std::mutex> _(m_msg_queue_lock);
        mLogDebug("Message '{}' sent : {}", m_msg_queue.front().first->message_name(), ec.message());
        if(ec) {
            m_msg_queue = {};
            return;
        }

        auto cb = m_msg_queue.front().second;
        if(cb) {
            boost::asio::post(m_io_context.get_executor(), std::bind(std::move(cb), ec));
        }
        m_msg_queue.erase(m_msg_queue.begin());
        write_next();
    }

    void write_next() {
        if(is_rebooting() && m_msg_queue.empty()) {
            return;
        }

        if(!m_msg_queue.empty() && !m_upgrading_firmware) {
            auto that = shared_from_this();
            auto cb =
                boost::asio::bind_executor(m_strand, [that](boost::system::error_code ec) { that->handle_write(ec); });

            auto& message = *(m_msg_queue.front().first);
            variant_ns::visit(overloaded{[](variant_ns::monostate&) {},
                                         [&cb, &message](auto& underlying) {
                                             mobsya::async_write_aseba_message(underlying, message, std::move(cb));
                                         }},
                              m_endpoint.ep());
        }
    }

    aseba_endpoint(boost::asio::io_context& io_context, aseba_device&& e, endpoint_type type = endpoint_type::thymio);

    const Aseba::CommonDefinitions& aseba_compiler_definitions() const {
        return m_defs;
    }

    void restore_firmware();

    bool is_rebooting() const {
        return m_upgrading_firmware || m_rebooting;
    }

    std::optional<std::pair<Aseba::EventDescription, std::size_t>> get_event(const std::string& name) const;
    std::optional<std::pair<Aseba::EventDescription, std::size_t>> get_event(uint16_t id) const;

    friend class group;
    boost::system::error_code set_events_table(const group::events_table& events);
    boost::system::error_code set_shared_variables(const variables_map& map);
    void emit_events(const group::properties_map& map, write_callback&& cb);
    void on_event(const Aseba::UserMessage& event, const Aseba::EventDescription& def,
                  const std::chrono::system_clock::time_point& timestamp);

    aseba_device m_endpoint;
    boost::asio::strand<boost::asio::io_context::executor_type> m_strand;
    boost::asio::io_service& m_io_context;
    endpoint_type m_endpoint_type;
    std::string m_endpoint_name;
    std::mutex m_msg_queue_lock;
    std::unordered_map<aseba_node::node_id_t, node_info> m_nodes;
    std::shared_ptr<mobsya::group> m_group;
    std::vector<std::pair<std::shared_ptr<Aseba::Message>, write_callback>> m_msg_queue;
    Aseba::CommonDefinitions m_defs;

    node_id m_uuid;

    bool m_upgrading_firmware = false;
    bool m_first_ping = true;
    bool m_rebooting = false;
};

}  // namespace mobsya
