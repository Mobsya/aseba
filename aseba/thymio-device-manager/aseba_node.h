#pragma once
#include <memory>
#include <mutex>
#include "aseba/common/msg/msg.h"
#include "node_id.h"
#include "property.h"
#include <boost/asio/io_context.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <atomic>
#include <aseba/flatbuffers/thymio_generated.h>
#include <boost/signals2.hpp>
#include <unordered_map>

namespace mobsya {
class aseba_endpoint;

class aseba_node : public std::enable_shared_from_this<aseba_node> {
public:
    // aseba_node::status is exposed through zero conf & protocol : needs to be stable
    enum class status { connected = 1, available = 2, busy = 3, ready = 4, disconnected = 5 };
    using node_type = fb::NodeType;

    using node_id_t = uint16_t;

    using variables_map = std::unordered_map<std::string, mobsya::property>;
    using variables_watch_signal_t = boost::signals2::signal<void(std::shared_ptr<aseba_node>, variables_map)>;


    aseba_node(boost::asio::io_context& ctx, node_id_t id, std::weak_ptr<mobsya::aseba_endpoint> endpoint);
    using write_callback = std::function<void(boost::system::error_code)>;

    ~aseba_node();

    static std::shared_ptr<aseba_node> create(boost::asio::io_context& ctx, node_id_t id,
                                              std::weak_ptr<mobsya::aseba_endpoint> endpoint);

    static const std::string& status_to_string(aseba_node::status);
    status get_status() const {
        return m_status.load();
    }

    node_id_t native_id() const {
        return m_id;
    }

    node_id uuid() const {
        return m_uuid;
    }

    node_type type() const;

    std::string friendly_name() const;
    void set_friendly_name(const std::string& str);
    bool can_be_renamed() const;

    bool is_wirelessly_connected() const;

    Aseba::TargetDescription vm_description() const {
        std::unique_lock<std::mutex> _(m_node_mutex);
        return m_description;
    }

    variables_map variables() const;

    // Write n messages to the enpoint owning that node, then invoke cb when all message have been written
    void write_messages(std::vector<std::shared_ptr<Aseba::Message>>&& message, write_callback&& cb = {});
    // Write a message to the enpoint owning that node, then invoke cb
    void write_message(std::shared_ptr<Aseba::Message> message, write_callback&& cb = {});

    // Compile a program and send it to the node, invoking cb once the assossiated message is written out
    // If the code can not be compiled, returns false without invoking cb
    bool send_aseba_program(const std::string& program, write_callback&& cb = {});
    void run_aseba_program(write_callback&& cb = {});
    boost::system::error_code set_node_variables(const aseba_node::variables_map & map, write_callback&& cb = {});
    void rename(const std::string& new_name);
    void stop_vm(write_callback&& cb = {});
    bool lock(void* app);
    bool unlock(void* app);

    template <typename... ConnectionArgs>
    auto connect_to_variables_changes(ConnectionArgs... args) {
        m_resend_all_variables = true;
        return m_variables_changed_signal.connect(std::forward<ConnectionArgs>(args)...);
    }

private:
    friend class aseba_endpoint;
    void set_status(status);

    // Must be called before destructor !
    void disconnect();
    void on_message(const Aseba::Message& msg);
    void on_description(Aseba::TargetDescription description);
    void on_device_info(const Aseba::DeviceInfo& info);

    void reset_known_variables(const Aseba::VariablesMap& variables);
    void request_variables();
    void on_variables_message(const Aseba::Variables& msg);
    void on_variables_message(const Aseba::ChangedVariables& msg);
    void set_variables(uint16_t start, const std::vector<int16_t>& data,
                       std::unordered_map<std::string, mobsya::property>& vars);
    void schedule_variables_update();

    node_id_t m_id;
    node_id m_uuid;
    std::string m_friendly_name;
    std::atomic<status> m_status;
    std::atomic<void*> m_connected_app;
    std::weak_ptr<mobsya::aseba_endpoint> m_endpoint;
    mutable std::mutex m_node_mutex;
    Aseba::TargetDescription m_description;
    boost::asio::io_context& m_io_ctx;

    struct variable {
        std::string name;
        uint16_t start;
        uint16_t size;
        std::vector<int16_t> value;

        variable(const std::string& name, uint16_t start, uint16_t size) : name(name), start(start), size(size) {}
    };
    std::vector<variable> m_variables;
    boost::asio::deadline_timer m_variables_timer;
    variables_watch_signal_t m_variables_changed_signal;
    std::atomic<bool> m_resend_all_variables = true;
};


}  // namespace mobsya
