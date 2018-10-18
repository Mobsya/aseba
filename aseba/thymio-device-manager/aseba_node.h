#pragma once
#include <memory>
#include <mutex>
#include <boost/asio/post.hpp>
#include "aseba/common/msg/msg.h"
#include "aseba/compiler/compiler.h"
#include "node_id.h"
#include "property.h"
#include "events.h"
#include <boost/asio/io_context.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <atomic>
#include <aseba/flatbuffers/thymio_generated.h>
#include <boost/signals2.hpp>
#include <unordered_map>
#include <unordered_set>

namespace mobsya {
class aseba_endpoint;

struct breakpoint {
    uint32_t line;
    breakpoint(uint32_t line) : line(line) {}
};

inline bool operator==(const breakpoint& a, const breakpoint& b) {
    return a.line == b.line;
}

}  // namespace mobsya

namespace std {
template <>
struct hash<mobsya::breakpoint> {
    auto operator()(mobsya::breakpoint const& b) const noexcept {
        return std::hash<uint32_t>()(b.line);
    }
};

}  // namespace std

namespace mobsya {

class aseba_node : public std::enable_shared_from_this<aseba_node> {
public:
    // aseba_node::status is exposed through zero conf & protocol : needs to be stable
    enum class status { connected = 1, available = 2, busy = 3, ready = 4, disconnected = 5 };
    using node_type = fb::NodeType;

    using node_id_t = uint16_t;

    struct variable {
        static const bool constant_tag = true;
        variable(property p) : value(std::move(p)) {}
        variable(property p, bool c) : value(std::move(p)), is_constant(c) {}
        mobsya::property value;
        bool is_constant = false;

        operator mobsya::property&() {
            return value;
        }
        operator const mobsya::property&() const {
            return value;
        }
    };

    struct vm_execution_state {
        fb::VMExecutionState state;
        uint32_t line;
        fb::VMExecutionError error;
        std::optional<std::string> error_message;
    };

    struct compilation_result {
        struct error_data {
            unsigned character;
            unsigned line;
            unsigned colum;
            std::string msg;
        };
        struct result_data {
            size_t bytecode_size;
            size_t variables_size;
            size_t bytecode_total_size;
            size_t variables_total_size;
        };
        std::optional<error_data> error;
        std::optional<result_data> result;
    };

    using breakpoints = std::unordered_set<breakpoint>;

    using variables_map = std::unordered_map<std::string, variable>;
    using variables_watch_signal_t = boost::signals2::signal<void(std::shared_ptr<aseba_node>, variables_map)>;

    using events_description_type = std::vector<mobsya::event>;
    using event_changed_payload = variant_ns::variant<events_description_type, variables_map>;
    using events_watch_signal_t = boost::signals2::signal<void(std::shared_ptr<aseba_node>, event_changed_payload)>;

    using vm_state_watch_signal_t = boost::signals2::signal<void(std::shared_ptr<aseba_node>, vm_execution_state)>;
    using vm_execution_state_command = fb::VMExecutionStateCommand;


    aseba_node(boost::asio::io_context& ctx, node_id_t id, std::weak_ptr<mobsya::aseba_endpoint> endpoint);
    using write_callback = std::function<void(boost::system::error_code)>;
    using breakpoints_callback = std::function<void(boost::system::error_code, breakpoints)>;
    using compilation_callback = std::function<void(boost::system::error_code, compilation_result)>;

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
    events_description_type events_description() const;
    vm_execution_state execution_state() const;

    // Write n messages to the enpoint owning that node, then invoke cb when all message have been written
    void write_messages(std::vector<std::shared_ptr<Aseba::Message>>&& message, write_callback&& cb = {});
    // Write a message to the enpoint owning that node, then invoke cb
    void write_message(std::shared_ptr<Aseba::Message> message, write_callback&& cb = {});

    // Compile a program and send it to the node, invoking cb once the assossiated message is written out
    void compile_program(fb::ProgrammingLanguage language, const std::string& program, compilation_callback&& cb = {});
    void compile_and_send_program(fb::ProgrammingLanguage language, const std::string& program,
                                  compilation_callback&& cb = {});
    void set_vm_execution_state(vm_execution_state_command state, write_callback&& cb = {});
    void set_breakpoints(std::vector<breakpoint> breakpoints, breakpoints_callback&& cb = {});

    boost::system::error_code set_node_variables(const aseba_node::variables_map& map, write_callback&& cb = {});
    boost::system::error_code emit_events(const aseba_node::variables_map& map, write_callback&& cb = {});
    void rename(const std::string& new_name);
    bool lock(void* app);
    bool unlock(void* app);

    template <typename... ConnectionArgs>
    auto connect_to_variables_changes(ConnectionArgs... args) {
        m_resend_all_variables = true;
        return m_variables_changed_signal.connect(std::forward<ConnectionArgs>(args)...);
    }

    template <typename... ConnectionArgs>
    auto connect_to_events(ConnectionArgs... args) {
        return m_events_signal.connect(std::forward<ConnectionArgs>(args)...);
    }

    template <typename... ConnectionArgs>
    auto connect_to_execution_state_changes(ConnectionArgs... args) {
        return m_vm_state_watch_signal.connect(std::forward<ConnectionArgs>(args)...);
    }

private:
    friend class aseba_endpoint;
    void set_status(status);
    tl::expected<compilation_result, boost::system::error_code>
    do_compile_program(Aseba::Compiler& compiler, Aseba::CommonDefinitions& defs, fb::ProgrammingLanguage language,
                       const std::string& program, Aseba::BytecodeVector& bytecode);

    // Must be called before destructor !
    void disconnect();
    void on_message(const Aseba::Message& msg);
    void on_description(Aseba::TargetDescription description);
    void on_device_info(const Aseba::DeviceInfo& info);
    void on_event(const Aseba::UserMessage& event, const Aseba::EventDescription& def);

    void reset_known_variables(const Aseba::VariablesMap& variables);
    void request_variables();
    void on_variables_message(const Aseba::Variables& msg);
    void on_variables_message(const Aseba::ChangedVariables& msg);
    void set_variables(uint16_t start, const std::vector<int16_t>& data,
                       std::unordered_map<std::string, variable>& vars);
    void schedule_variables_update();
    void send_events_table();
    void on_execution_state_message(const Aseba::ExecutionStateChanged&);
    void on_vm_runtime_error(const Aseba::Message&);
    void force_execution_state_update();

    void on_breakpoint_set_result(const Aseba::BreakpointSetResult&);
    void cancel_pending_breakpoint_request();


    std::optional<std::pair<Aseba::EventDescription, std::size_t>> get_event(const std::string& name) const;
    std::optional<std::pair<Aseba::EventDescription, std::size_t>> get_event(uint16_t id) const;

    node_id_t m_id;
    node_id m_uuid;
    std::string m_friendly_name;
    std::atomic<status> m_status;
    std::atomic<void*> m_connected_app;
    std::weak_ptr<mobsya::aseba_endpoint> m_endpoint;
    mutable std::mutex m_node_mutex;
    Aseba::TargetDescription m_description;
    Aseba::CommonDefinitions m_defs;
    Aseba::BytecodeVector m_bytecode;
    breakpoints m_breakpoints;
    boost::asio::io_context& m_io_ctx;

    struct {
        int pc = 0;
        int flags = 0;
        uint32_t line = 0;
        fb::VMExecutionState state = fb::VMExecutionState::Stopped;
    } m_vm_state;

    struct aseba_vm_variable {
        std::string name;
        uint16_t start;
        uint16_t size;
        std::vector<int16_t> value;

        aseba_vm_variable(const std::string& name, uint16_t start, uint16_t size)
            : name(name), start(start), size(size) {}
    };
    std::vector<aseba_vm_variable> m_variables;
    boost::asio::deadline_timer m_variables_timer;
    variables_watch_signal_t m_variables_changed_signal;
    events_watch_signal_t m_events_signal;
    vm_state_watch_signal_t m_vm_state_watch_signal;
    std::atomic<bool> m_resend_all_variables = true;


    struct break_point_cb_data {
        std::map<unsigned, breakpoint> pending;  // pc -> line
        breakpoints set;
        boost::system::error_code error;
        breakpoints_callback cb;
    };
    std::shared_ptr<break_point_cb_data> m_pending_breakpoint_request;
};

}  // namespace mobsya
