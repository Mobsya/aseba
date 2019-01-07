#include "aseba_node.h"
#include "aseba_endpoint.h"
#include "aseba_node_registery.h"
#include <aseba/common/utils/utils.h>
#include <aseba/compiler/compiler.h>
#include <fmt/format.h>
#include "aesl_parser.h"

namespace mobsya {

static const uint32_t MAX_FRIENDLY_NAME_SIZE = 30;

namespace detail {
    template <typename Rng>
    static auto aseba_variable_from_range(Rng&& rng) {
        if(rng.size() == 0)
            return property();
        return property(property::list::from_range(std::forward<Rng>(rng)));
    }
}  // namespace detail

const std::string& aseba_node::status_to_string(aseba_node::status s) {
    static std::array<std::string, 5> strs = {"connected", "available", "busy", "ready", "disconnected"};
    int i = int(s) - 1;
    if(i < 0 && i >= int(status::connected))
        return strs[4];
    return strs[i];
}


aseba_node::aseba_node(boost::asio::io_context& ctx, node_id_t id, uint16_t protocol_version,
                       std::weak_ptr<mobsya::aseba_endpoint> endpoint)
    : m_id(id)
    , m_uuid(boost::uuids::random_generator()())
    , m_status(status::disconnected)
    , m_protocol_version(protocol_version)
    , m_connected_app(nullptr)
    , m_endpoint(std::move(endpoint))
    , m_io_ctx(ctx)
    , m_variables_timer(ctx)
    , m_status_timer(ctx)
    , m_resend_timer(ctx) {}

std::shared_ptr<aseba_node> aseba_node::create(boost::asio::io_context& ctx, node_id_t id, uint16_t protocol_version,
                                               std::weak_ptr<mobsya::aseba_endpoint> endpoint) {
    auto node = std::make_shared<aseba_node>(ctx, id, protocol_version, std::move(endpoint));
    node->set_status(status::connected);
    return node;
}


void aseba_node::disconnect() {
    cancel_pending_step_request();
    cancel_pending_breakpoint_request();
    set_status(status::disconnected);
}

aseba_node::~aseba_node() {
    cancel_pending_step_request();
    cancel_pending_breakpoint_request();
    if(m_status.load() != status::disconnected) {
        mLogWarn("Node destroyed before being disconnected");
    }
}

bool aseba_node::is_wirelessly_connected() const {
    if(auto endpoint = m_endpoint.lock())
        return endpoint->is_wireless();
    return false;
}

void aseba_node::on_message(const Aseba::Message& msg) {
    switch(msg.type) {
        case ASEBA_MESSAGE_DEVICE_INFO: {
            on_device_info(static_cast<const Aseba::DeviceInfo&>(msg));
            break;
        }
        case ASEBA_MESSAGE_VARIABLES: {
            on_variables_message(static_cast<const Aseba::Variables&>(msg));
            break;
        }
        case ASEBA_MESSAGE_CHANGED_VARIABLES: {
            on_variables_message(static_cast<const Aseba::ChangedVariables&>(msg));
            break;
        }
        case ASEBA_MESSAGE_EXECUTION_STATE_CHANGED: {
            on_execution_state_message(static_cast<const Aseba::ExecutionStateChanged&>(msg));
            break;
        }
        case ASEBA_MESSAGE_ARRAY_ACCESS_OUT_OF_BOUNDS:
        case ASEBA_MESSAGE_DIVISION_BY_ZERO:
        case ASEBA_MESSAGE_EVENT_EXECUTION_KILLED:
        case ASEBA_MESSAGE_NODE_SPECIFIC_ERROR: on_vm_runtime_error(msg); break;
        case ASEBA_MESSAGE_BREAKPOINT_SET_RESULT:
            on_breakpoint_set_result(static_cast<const Aseba::BreakpointSetResult&>(msg));
            break;

        case ASEBA_MESSAGE_DESCRIPTION:
        case ASEBA_MESSAGE_NAMED_VARIABLE_DESCRIPTION:
        case ASEBA_MESSAGE_NATIVE_FUNCTION_DESCRIPTION:
        case ASEBA_MESSAGE_LOCAL_EVENT_DESCRIPTION: {
            handle_description_messages(msg);
            break;
        }

        default: {
            if(msg.type >= 0x8000)  // first non-event message
                break;
            auto event = [this, &msg] { return get_event(msg.type); }();
            if(event) {
                on_event(static_cast<const Aseba::UserMessage&>(msg), event->first);
            }
        }
    }
}

void aseba_node::set_status(status s) {
    m_status = s;
    auto& registery = boost::asio::use_service<aseba_node_registery>(m_io_ctx);
    switch(s) {
        case status::connected: registery.add_node(shared_from_this()); break;
        case status::disconnected: registery.remove_node(shared_from_this()); break;
        default: registery.set_node_status(shared_from_this(), s); break;
    }
}

bool aseba_node::lock(void* app) {
    if(m_connected_app == app) {
        return true;
    }
    if(m_connected_app != nullptr || m_status != status::available) {
        return false;
    }
    m_connected_app = app;
    set_status(status::busy);
    return true;
}

bool aseba_node::unlock(void* app) {
    if(m_connected_app != app) {
        return false;
    }
    m_connected_app = nullptr;
    mLogDebug("Unlocking node");
    set_status(status::available);
    return true;
}

void aseba_node::write_message(std::shared_ptr<Aseba::Message> message, write_callback&& cb) {
    write_messages({{std::move(message)}}, std::move(cb));
}

void aseba_node::write_messages(std::vector<std::shared_ptr<Aseba::Message>>&& messages, write_callback&& cb) {
    auto endpoint = m_endpoint.lock();
    if(!endpoint) {
        return;
    }
    endpoint->write_messages(std::move(messages), std::move(cb));
}

void aseba_node::compile_program(fb::ProgrammingLanguage language, const std::string& program,
                                 compilation_callback&& cb) {
    Aseba::CommonDefinitions defs = m_defs;
    Aseba::TargetDescription desc = m_description;

    Aseba::Compiler compiler;
    compiler.setTargetDescription(&desc);
    compiler.setCommonDefinitions(&defs);


    Aseba::BytecodeVector bytecode;
    auto result = do_compile_program(compiler, defs, language, program, bytecode);
    if(!result)
        boost::asio::post(m_io_ctx.get_executor(), std::bind(std::move(cb), result.error(), compilation_result{}));
    else
        boost::asio::post(m_io_ctx.get_executor(),
                          std::bind(std::move(cb), boost::system::error_code{}, result.value()));
}

void aseba_node::compile_and_send_program(fb::ProgrammingLanguage language, const std::string& program,
                                          compilation_callback&& cb) {
    m_breakpoints.clear();
    cancel_pending_step_request();
    cancel_pending_breakpoint_request();
    Aseba::Compiler compiler;
    compiler.setTargetDescription(&m_description);
    compiler.setCommonDefinitions(&m_defs);
    auto result = do_compile_program(compiler, m_defs, language, program, m_bytecode);
    if(!result) {
        cb(result.error(), {});
        return;
    }
    std::vector<std::shared_ptr<Aseba::Message>> messages;
    Aseba::sendBytecode(messages, native_id(), std::vector<uint16_t>(m_bytecode.begin(), m_bytecode.end()));
    reset_known_variables(*compiler.getVariablesMap());
    write_messages(std::move(messages),
                   [that = shared_from_this(), cb = std::move(cb), result](boost::system::error_code ec) {
                       if(ec)
                           cb(ec, result.value());
                       else
                           that->m_callbacks_pending_execution_state_change.push(std::bind(cb, ec, result.value()));
                   });

    send_events_table();
    m_variables_changed_signal(shared_from_this(), this->variables());
}

tl::expected<aseba_node::compilation_result, boost::system::error_code>
aseba_node::do_compile_program(Aseba::Compiler& compiler, Aseba::CommonDefinitions& defs,
                               fb::ProgrammingLanguage language, const std::string& program,
                               Aseba::BytecodeVector& bytecode) {
    std::wstring code;

    if(language == fb::ProgrammingLanguage::Aesl) {
        auto aesl = load_aesl(program);
        if(!aesl) {
            mLogError("Invalid Aesl");
            return tl::make_unexpected(mobsya::make_error_code(mobsya::error_code::invalid_aesl));
        }
        auto [constants, events, nodes] = aesl->parse_all();
        if(!constants || !events || !nodes || nodes->size() != 1) {
            mLogError("Invalid Aesl");
            return tl::make_unexpected(mobsya::make_error_code(mobsya::error_code::invalid_aesl));
        }

        for(const auto& constant : *constants) {
            // Let poorly defined constants fail the compilation later
            if(!constant.second.is_integral())
                continue;
            auto v = numeric_cast<int16_t>(property::integral_t(constant.second));
            if(v)
                defs.constants.emplace_back(Aseba::UTF8ToWString(constant.first), *v);
        }
        for(const auto& event : *events) {
            // Let poorly defined events fail the compilation later
            if(event.type != event_type::aseba)
                continue;
            defs.events.emplace_back(Aseba::UTF8ToWString(event.name), event.size);
        }
        code = Aseba::UTF8ToWString((*nodes)[0].code);
    } else {
        code = Aseba::UTF8ToWString(program);
    }

    compilation_result result;
    std::wistringstream is(code);
    Aseba::Error error;
    bytecode.clear();
    unsigned allocatedVariablesCount;
    bool success = compiler.compile(is, bytecode, allocatedVariablesCount, error);
    if(!success) {
        mLogWarn("Compilation failed on node {} : {}", m_id, Aseba::WStringToUTF8(error.message));
        compilation_result::error_data err{error.pos.character, error.pos.row, error.pos.column,
                                           Aseba::WStringToUTF8(error.message)};
        result.error = err;
        return result;
    }
    compilation_result::result_data data;
    data.bytecode_size = bytecode.size();
    data.variables_size = allocatedVariablesCount;
    data.bytecode_total_size = compiler.getTargetDescription()->bytecodeSize;
    data.variables_total_size = compiler.getTargetDescription()->variablesSize;
    result.result = data;
    return result;
}

void aseba_node::set_vm_execution_state(vm_execution_state_command state, write_callback&& cb) {
    switch(state) {
        case vm_execution_state_command::Run:
            write_message(std::make_shared<Aseba::Run>(native_id()), std::move(cb));
            break;
        case vm_execution_state_command::Reset:
            write_message(std::make_shared<Aseba::Reset>(native_id()), std::move(cb));
            break;
        case vm_execution_state_command::Stop:
            write_message(std::make_shared<Aseba::Stop>(native_id()), std::move(cb));
            break;
        case vm_execution_state_command::Pause:
            write_message(std::make_shared<Aseba::Pause>(native_id()), std::move(cb));
            break;
        case vm_execution_state_command::StepToNextLine: step_to_next_line(std::move(cb)); break;
        case vm_execution_state_command::Step:
            write_message(std::make_shared<Aseba::Step>(native_id()), std::move(cb));
            break;
        case vm_execution_state_command::Suspend:
            write_message(std::make_shared<Aseba::Sleep>(native_id()), std::move(cb));
            break;
        case vm_execution_state_command::Reboot:
            write_message(std::make_shared<Aseba::Reboot>(native_id()), std::move(cb));
            break;
        case vm_execution_state_command::WriteProgramToDeviceMemory:
            write_message(std::make_shared<Aseba::WriteBytecode>(native_id()), std::move(cb));
            break;
    }
}


void aseba_node::schedule_execution_state_update() {
    request_execution_state();
	m_status_timer.expires_from_now(boost::posix_time::seconds(1));
    std::weak_ptr<aseba_node> ptr = shared_from_this();
    m_status_timer.async_wait([ptr](boost::system::error_code ec) {
        if(ec)
            return;
        auto that = ptr.lock();
        if(!that || that->get_status() == status::disconnected)
            return;
        that->schedule_execution_state_update();
    });
}

void aseba_node::request_execution_state() {
    write_message(std::make_shared<Aseba::GetExecutionState>(native_id()));
}


void aseba_node::on_execution_state_message(const Aseba::ExecutionStateChanged& es) {
    vm_execution_state state;

    while(!m_callbacks_pending_execution_state_change.empty()) {
        boost::asio::post(m_io_ctx.get_executor(), std::move(m_callbacks_pending_execution_state_change.front()));
        m_callbacks_pending_execution_state_change.pop();
    }

    m_vm_state.state = fb::VMExecutionState::Running;
    m_vm_state.pc = es.pc;
    m_vm_state.line = line_from_pc(es.pc);
    m_vm_state.flags = es.flags;
    if(m_vm_state.flags & ASEBA_VM_STEP_BY_STEP_MASK) {
        m_vm_state.state = (m_vm_state.flags & ASEBA_VM_EVENT_ACTIVE_MASK) ? fb::VMExecutionState::Paused :
                                                                             fb::VMExecutionState::Stopped;
    }

    state.state = m_vm_state.state;
    state.line = m_vm_state.line;
    state.error = fb::VMExecutionError::NoError;

    handle_step_request();
    if(m_pending_step_request)
        return;

    m_vm_state_watch_signal(shared_from_this(), state);
}

void aseba_node::on_vm_runtime_error(const Aseba::Message& msg) {
    cancel_pending_step_request();
    vm_execution_state state;
    int pc;
    switch(msg.type) {
        case ASEBA_MESSAGE_ARRAY_ACCESS_OUT_OF_BOUNDS: {
            auto message = static_cast<const Aseba::ArrayAccessOutOfBounds&>(msg);
            state.error = fb::VMExecutionError::OutOfBoundAccess;
            state.error_message = "Out of bound access";
            pc = message.pc;
            break;
        }
        case ASEBA_MESSAGE_DIVISION_BY_ZERO: {
            auto message = static_cast<const Aseba::DivisionByZero&>(msg);
            state.error = fb::VMExecutionError::DivisionByZero;
            state.error_message = "Division by 0";
            pc = message.pc;
            break;
        }
        case ASEBA_MESSAGE_EVENT_EXECUTION_KILLED: {
            auto message = static_cast<const Aseba::EventExecutionKilled&>(msg);
            state.error = fb::VMExecutionError::Killed;
            state.error_message = "VM Execution killed";
            pc = message.pc;
            break;
        }
        case ASEBA_MESSAGE_NODE_SPECIFIC_ERROR: {
            auto message = static_cast<const Aseba::NodeSpecificError&>(msg);
            state.error = fb::VMExecutionError::GenericError;
            state.error_message = Aseba::WStringToUTF8(message.message);
            pc = message.pc;
            break;
        }
    }

    m_vm_state.pc = pc;
    m_vm_state.state = fb::VMExecutionState::Stopped;
    m_vm_state.line = line_from_pc(pc);
    state.state = m_vm_state.state;
    state.line = m_vm_state.line;

    m_vm_state_watch_signal(shared_from_this(), state);
}

unsigned aseba_node::line_from_pc(unsigned pc) const {
    return (pc >= 5 && pc < m_bytecode.size()) ? m_bytecode[pc].line + 1 : 0;
}

void aseba_node::step_to_next_line(write_callback&& cb) {
    cancel_pending_step_request();
    m_pending_step_request = std::make_shared<step_cb_data>();
    m_pending_step_request->current_pc = m_vm_state.pc;
    m_pending_step_request->current_line = m_vm_state.line;
    m_pending_step_request->cb = std::move(cb);
    handle_step_request();
}

void aseba_node::handle_step_request() {
    if(!m_pending_step_request)
        return;
    if(m_vm_state.state != fb::VMExecutionState::Paused) {
        cancel_pending_step_request();
        return;
    }
    if(m_vm_state.line != m_pending_step_request->current_line) {
        boost::asio::post(m_io_ctx.get_executor(),
                          std::bind(std::move(m_pending_step_request->cb), m_pending_step_request->error));
        m_pending_step_request.reset();
        return;
    }
    auto write_cb = [that = shared_from_this(),
                     ptr = std::weak_ptr<step_cb_data>(m_pending_step_request)](boost::system::error_code ec) {
        if(!ec) {
            return;
        }
        auto data = ptr.lock();
        if(!data)
            return;
        data->error = ec;
        that->cancel_pending_step_request();
    };
    write_message(std::make_shared<Aseba::Step>(native_id()), std::move(write_cb));
};

void aseba_node::cancel_pending_step_request() {
    if(m_pending_step_request && m_pending_step_request->cb) {
        if(!m_pending_step_request->error)
            m_pending_step_request->error =
                boost::system::errc::make_error_code(boost::system::errc::operation_canceled);

        boost::asio::post(m_io_ctx.get_executor(),
                          std::bind(std::move(m_pending_step_request->cb), m_pending_step_request->error));
    }
    m_pending_step_request.reset();
}

void aseba_node::set_breakpoints(std::vector<breakpoint> breakpoints, breakpoints_callback&& cb) {
    std::vector<std::shared_ptr<Aseba::Message>> messages;
    auto cb_data = std::make_shared<break_point_cb_data>();


    std::vector<unsigned> pc;
    cb_data->cb = std::move(cb);

    auto pc_from_line = [this](int line) -> int {
        for(std::size_t i = 0; i < m_bytecode.size(); i++) {
            if(m_bytecode[i].line == line - 1)
                return i;
        }
        return -1;
    };

    if(breakpoints.empty()) {
        messages.push_back(std::make_shared<Aseba::BreakpointClearAll>(native_id()));
    } else {
        for(auto b : breakpoints) {
            if(m_breakpoints.count(b)) {
                cb_data->set.insert(b);
            } else {
                auto pc = pc_from_line(b.line);
                if(pc >= 0) {
                    messages.push_back(std::make_shared<Aseba::BreakpointSet>(native_id(), pc));
                    cb_data->pending.insert_or_assign(pc, b);
                }
            }
        }
        for(auto b : m_breakpoints) {
            auto pc = pc_from_line(b.line);
            if(pc >= 0 && std::find(breakpoints.begin(), breakpoints.end(), b) == breakpoints.end())
                messages.push_back(std::make_shared<Aseba::BreakpointClear>(native_id(), pc));
        }
    }
    m_breakpoints = aseba_node::breakpoints(breakpoints.begin(), breakpoints.end());
    cancel_pending_breakpoint_request();
    if(cb_data->cb)
        m_pending_breakpoint_request = cb_data;

    auto write_cb = [that = shared_from_this(),
                     ptr = std::weak_ptr<break_point_cb_data>(cb_data)](boost::system::error_code ec) {
        auto data = ptr.lock();
        if(!data)
            return;
        data->error = ec;
        if(ec || data->pending.empty()) {
            that->m_breakpoints = data->set;
            that->m_pending_breakpoint_request.reset();
            boost::asio::post(that->m_io_ctx.get_executor(), std::bind(std::move(data->cb), data->error, data->set));
        }
    };
    if(messages.empty()) {
        m_breakpoints = cb_data->set;
        m_pending_breakpoint_request.reset();
        boost::asio::post(m_io_ctx.get_executor(), std::bind(std::move(cb_data->cb), cb_data->error, cb_data->set));
    }
    write_messages(std::move(messages), std::move(write_cb));
}

void aseba_node::on_breakpoint_set_result(const Aseba::BreakpointSetResult& res) {
    if(!m_pending_breakpoint_request)
        return;
    auto& r = *m_pending_breakpoint_request;
    auto it = r.pending.find(res.pc);
    if(it == std::end(r.pending))
        return;
    if(res.success)
        r.set.insert(it->second);
    r.pending.erase(it);
    if(r.pending.empty()) {
        m_breakpoints = r.set;
        boost::asio::post(m_io_ctx.get_executor(), std::bind(std::move(r.cb), r.error, r.set));
        m_pending_breakpoint_request.reset();
    }
}

void aseba_node::cancel_pending_breakpoint_request() {
    if(m_pending_breakpoint_request && m_pending_breakpoint_request->cb) {
        boost::asio::post(m_io_ctx.get_executor(),
                          std::bind(std::move(m_pending_breakpoint_request->cb),
                                    boost::system::errc::make_error_code(boost::system::errc::operation_canceled),
                                    breakpoints{}));
    }
    m_pending_breakpoint_request.reset();
}

aseba_node::events_table aseba_node::events_description() const {
    std::vector<mobsya::event> table;
    const auto& events = m_defs.events;
    table.reserve(events.size());
    for(const auto& e : events) {
        table.emplace_back(Aseba::WStringToUTF8(e.name), e.value);
    }
    return table;
}

aseba_node::vm_execution_state aseba_node::execution_state() const {
    return {m_vm_state.state, m_vm_state.line};
}

void aseba_node::send_events_table() {
    m_events_signal(shared_from_this(), events_description());
}

static tl::expected<std::vector<int16_t>, boost::system::error_code> to_aseba_variable(const property& p,
                                                                                       uint16_t size) {
    if(p.is_null() && size == 0) {
        return {};
    }
    if(size == 1 && p.is_integral()) {
        property::integral_t v(p);
        auto n = numeric_cast<int16_t>(v);
        if(!n)
            return make_unexpected(error_code::incompatible_variable_type);
        return std::vector({*n});
    }
    if(p.is_array() && size == p.size()) {
        std::vector<int16_t> vars;
        vars.reserve(p.size());
        for(std::size_t i = 0; i < p.size(); i++) {
            auto e = p[i];
            if(!e.is_integral()) {
                return make_unexpected(error_code::incompatible_variable_type);
            }
            auto v = property::integral_t(e);
            auto n = numeric_cast<int16_t>(v);
            if(!n)
                return make_unexpected(error_code::incompatible_variable_type);
            vars.push_back(*n);
        }
        return vars;
    }
    return make_unexpected(error_code::incompatible_variable_type);
}

boost::system::error_code aseba_node::set_node_variables(const aseba_node::variables_map& map, write_callback&& cb) {
    std::vector<std::shared_ptr<Aseba::Message>> messages;
    messages.reserve(map.size());
    variables_map modified;
    {
        for(auto&& var : map) {
            if(var.second.is_constant) {
                auto n = Aseba::UTF8ToWString(var.first);
                const auto& p = var.second.value;
                auto it = std::find_if(m_defs.constants.begin(), m_defs.constants.end(),
                                       [n](const Aseba::NamedValue& v) { return n == v.name; });
                if(it != m_defs.constants.end()) {
                    m_defs.constants.erase(it);
                    modified.emplace(var.first, var.second);
                }
                if(p.is_null())
                    continue;
                if(p.is_integral()) {
                    auto v = numeric_cast<int16_t>(property::integral_t(p));
                    if(v) {
                        m_defs.constants.emplace_back(n, *v);
                        modified.emplace(var.first, var.second);
                        continue;
                    }
                }
                return make_error_code(error_code::incompatible_variable_type);
            }

            auto it = std::find_if(m_variables.begin(), m_variables.end(),
                                   [name = var.first](const aseba_vm_variable& v) { return v.name == name; });
            if(it == std::end(m_variables)) {
                return make_error_code(error_code::no_such_variable);
            }
            const auto& object = *it;
            auto bytes = to_aseba_variable(var.second, object.size);
            if(!bytes) {
                return bytes.error();
            }
            auto msg = std::make_shared<Aseba::SetVariables>(native_id(), object.start, bytes.value());
            messages.push_back(std::move(msg));
        }
    }
    write_messages(std::move(messages), std::move(cb));
    if(!modified.empty()) {
        m_variables_changed_signal(shared_from_this(), modified);
    }
    return {};
}

boost::system::error_code aseba_node::set_node_events_table(const aseba_node::events_table& events) {
    m_defs.events.clear();
    for(const auto& event : events) {
        if(event.type != event_type::aseba)
            continue;
        m_defs.events.emplace_back(Aseba::UTF8ToWString(event.name), event.size);
    }
    send_events_table();
    return {};
}


boost::system::error_code aseba_node::emit_events(const aseba_node::variables_map& map, write_callback&& cb) {
    std::vector<std::shared_ptr<Aseba::Message>> messages;
    messages.reserve(map.size());
    {
        for(auto&& event : map) {
            auto event_def = get_event(event.first);
            if(!event_def) {
                return make_error_code(error_code::no_such_variable);
            }
            auto bytes = to_aseba_variable(event.second, event_def->first.value);
            if(!bytes) {
                return bytes.error();
            }
            auto msg = std::make_shared<Aseba::UserMessage>(event_def->second, bytes.value());
            messages.push_back(std::move(msg));
        }
    }
    write_messages(std::move(messages), std::move(cb));
    return {};
}

void aseba_node::rename(const std::string& newName) {
    set_friendly_name(newName);
    write_message(std::make_shared<Aseba::GetDeviceInfo>(native_id(), DEVICE_INFO_NAME));
}

void aseba_node::on_description_received() {
    mLogInfo("Got description for {} [{} variables, {} functions, {} events - protocol {}]", native_id(),
             m_description.namedVariables.size(), m_description.nativeFunctions.size(),
             m_description.localEvents.size(), m_description.protocolVersion);
    unsigned count;
    reset_known_variables(m_description.getVariablesMap(count));
    schedule_variables_update();
    schedule_execution_state_update();

    if(m_description.protocolVersion >= 6 &&
       (type() == aseba_node::node_type::Thymio2 || (type() == aseba_node::node_type::Thymio2Wireless))) {
        write_message(std::make_shared<Aseba::GetDeviceInfo>(native_id(), DEVICE_INFO_NAME));
        write_message(std::make_shared<Aseba::GetDeviceInfo>(native_id(), DEVICE_INFO_UUID));
        return;
    }
    set_status(status::available);
}


void aseba_node::request_device_info() {
    if(m_uuid_received)
        return;
    write_message(std::make_shared<Aseba::GetDeviceInfo>(native_id(), DEVICE_INFO_NAME));
    write_message(std::make_shared<Aseba::GetDeviceInfo>(native_id(), DEVICE_INFO_UUID));

    m_resend_timer.expires_from_now(boost::posix_time::seconds(1));
    m_resend_timer.async_wait([ptr = weak_from_this()](boost::system::error_code ec) {
        if(ec)
            return;
        auto that = ptr.lock();
        if(!that)
            return;
        that->request_device_info();
    });
}

// ask the node to dump its memory.
void aseba_node::request_variables() {

    std::vector<std::shared_ptr<Aseba::Message>> messages;
    messages.reserve(3);

    {
        if(!m_resend_all_variables && m_description.protocolVersion >= 7) {
            messages.emplace_back(std::make_shared<Aseba::GetChangedVariables>(native_id()));
        } else {
            uint16_t start = 0;
            uint16_t size = 0;
            for(const auto& var : m_variables) {
                if(size + var.size > ASEBA_MAX_EVENT_ARG_COUNT - 2) {  // cut at variable boundaries
                    messages.emplace_back(std::make_shared<Aseba::GetVariables>(native_id(), start, size));
                    start += size;
                    size = 0;
                }
                size += var.size;
            }
            if(size > 0)
                messages.emplace_back(std::make_shared<Aseba::GetVariables>(native_id(), start, size));
            m_resend_all_variables = false;
        }
    }
    write_messages(std::move(messages));
}

void aseba_node::reset_known_variables(const Aseba::VariablesMap& variables) {
    m_variables.clear();
    for(const auto& var : variables) {
        const auto name = Aseba::WStringToUTF8(var.first);
        const auto start = var.second.first;
        const auto size = var.second.second;
        auto insert_point =
            std::lower_bound(m_variables.begin(), m_variables.end(), start,
                             [](const aseba_vm_variable& v, unsigned start) { return v.start < start; });
        if(insert_point == m_variables.end() || insert_point->start != start) {
            m_variables.emplace(insert_point, name, start, size);
        }
    }
    m_resend_all_variables = true;
}

void aseba_node::on_variables_message(const Aseba::Variables& msg) {
    std::unordered_map<std::string, variable> changed;
    set_variables(msg.start, msg.variables, changed);
    m_variables_changed_signal(shared_from_this(), changed);
    schedule_variables_update();
}

void aseba_node::on_variables_message(const Aseba::ChangedVariables& msg) {
    std::unordered_map<std::string, variable> changed;
    for(const auto& area : msg.variables) {
        set_variables(area.start, area.variables, changed);
    }
    m_variables_changed_signal(shared_from_this(), changed);
    schedule_variables_update();
}

void aseba_node::set_variables(uint16_t start, const std::vector<int16_t>& data,
                               std::unordered_map<std::string, variable>& vars) {

    auto data_it = std::begin(data);
    auto it = m_variables.begin();
    while(data_it != std::end(data)) {
        it = std::find_if(it, m_variables.end(), [start](const aseba_vm_variable& var) {
            return start >= var.start && start < var.start + var.size;
        });
        if(it == std::end(m_variables))
            return;
        auto& var = *it;
        const auto var_start = start - var.start;
        bool force_change = var.size != var.value.size();
        var.value.resize(var.size, 0);
        const auto count = std::min(std::ptrdiff_t(var.size - var_start), std::distance(data_it, std::end(data)));
        if(count < 0)
            break;
        if(force_change ||
           !std::equal(std::begin(var.value) + var_start, std::begin(var.value) + var_start + count, data_it,
                       data_it + count)) {
            std::copy(data_it, data_it + count, std::begin(var.value) + var_start);
            vars.insert(std::pair{var.name, detail::aseba_variable_from_range(var.value)});
            mLogTrace("Variable changed {} : {}", var.name, detail::aseba_variable_from_range(var.value));
        }
        data_it = data_it + count;
        start += count;
    }
}


aseba_node::variables_map aseba_node::variables() const {
    variables_map map;
    map.reserve(m_variables.size());
    for(auto& var : m_variables) {
        map.emplace(var.name, detail::aseba_variable_from_range(var.value));
    }
    for(auto& var : m_defs.constants) {
        map.emplace(Aseba::WStringToUTF8(var.name), variable(var.value, variable::constant_tag));
    }
    return map;
}

void aseba_node::schedule_variables_update(boost::posix_time::time_duration delay) {
    m_variables_timer.expires_from_now(delay);
    std::weak_ptr<aseba_node> ptr = shared_from_this();
    m_variables_timer.async_wait([ptr](boost::system::error_code ec) {
        if(ec)
            return;
        auto that = ptr.lock();
        if(!that || that->get_status() == status::disconnected)
            return;

        // Only ask variables if we have at least 1 watcher
        if(!that->m_variables_changed_signal.empty())
            that->request_variables();

        // schedule_variables_update is called in on_variables_message
        // every 100ms or so
        // However, the packet might be dropped, so this is a fail safe to make
        // sure we ask for variables at least once every second
        that->schedule_variables_update(boost::posix_time::seconds(1));
    });
}

void aseba_node::on_device_info(const Aseba::DeviceInfo& info) {
    mLogTrace("Got info for {} [{} : {}]", native_id(), info.info, info.data.size());
    if(info.info == DEVICE_INFO_UUID) {
        // see request_device_info
        m_resend_timer.cancel();

        if(info.data.size() == 16) {
            std::copy(info.data.begin(), info.data.end(), m_uuid.begin());
        }
        bool save_uuid = m_uuid.is_nil() || info.data.size() != 16;
        if(m_uuid.is_nil()) {
            m_uuid = boost::uuids::random_generator()();
        }
        if(save_uuid) {
            std::vector<uint8_t> data;
            std::copy(m_uuid.begin(), m_uuid.end(), std::back_inserter(data));
            write_message(std::make_shared<Aseba::SetDeviceInfo>(native_id(), DEVICE_INFO_UUID, data));
        }
        mLogInfo("Persistent uuid for {} is now {} ", native_id(), boost::uuids::to_string(m_uuid));
        auto& registery = boost::asio::use_service<aseba_node_registery>(m_io_ctx);
        registery.set_node_uuid(shared_from_this(), m_uuid);
        set_status(status::available);

    } else if(info.info == DEVICE_INFO_NAME) {
        m_friendly_name.clear();
        m_friendly_name.reserve(info.data.size());
        std::copy(info.data.begin(), info.data.end(), std::back_inserter(m_friendly_name));
        set_status(m_status);
        mLogInfo("Persistent name for {} is now \"{}\"", native_id(), friendly_name());
    }
}

void aseba_node::on_event(const Aseba::UserMessage& event, const Aseba::EventDescription& def) {
    variables_map events;
    auto p = detail::aseba_variable_from_range(event.data);
    if((p.is_integral() && def.value != 1) || p.size() != std::size_t(def.value))
        return;
    events.insert(std::pair{Aseba::WStringToUTF8(def.name), p});
    m_events_signal(shared_from_this(), events);
}

aseba_node::node_type aseba_node::type() const {
    auto ep = m_endpoint.lock();
    if(!ep) {
        return aseba_node::node_type::UnknownType;
    }
    switch(ep->type()) {
        case aseba_endpoint::endpoint_type::thymio:
            return is_wirelessly_connected() ? node_type::Thymio2Wireless : node_type::Thymio2;
        case aseba_endpoint::endpoint_type::simulated_thymio: return node_type::SimulatedThymio2;
        case aseba_endpoint::endpoint_type::simulated_dummy_node: return node_type::DummyNode;
        default: break;
    }
    return node_type::UnknownType;
}

std::string aseba_node::friendly_name() const {
    if(m_friendly_name.empty()) {
        auto ep = m_endpoint.lock();
        if(ep) {
            return ep->endpoint_name();
        }
    }
    return m_friendly_name;
}

void aseba_node::set_friendly_name(const std::string& str) {
    if(str.empty() || str.size() > MAX_FRIENDLY_NAME_SIZE)
        return;
    std::vector<uint8_t> data;
    data.reserve(str.size());
    std::copy(str.begin(), str.end(), std::back_inserter(data));
    write_message(std::make_shared<Aseba::SetDeviceInfo>(native_id(), DEVICE_INFO_NAME, data));
    m_friendly_name = str;
}

bool aseba_node::can_be_renamed() const {
    auto ep = m_endpoint.lock();
    return ep && ep->type() == aseba_endpoint::endpoint_type::thymio && m_description.protocolVersion >= 6;
}

std::optional<std::pair<Aseba::EventDescription, std::size_t>> aseba_node::get_event(const std::string& name) const {
    auto wname = Aseba::UTF8ToWString(name);
    for(std::size_t i = 0; i < m_defs.events.size(); i++) {
        const auto& event = m_defs.events[i];
        if(event.name == wname) {
            return std::optional<std::pair<Aseba::EventDescription, std::size_t>>(std::pair{event, i});
        }
    }
    return {};
};

std::optional<std::pair<Aseba::EventDescription, std::size_t>> aseba_node::get_event(uint16_t id) const {
    if(m_defs.events.size() <= id)
        return {};
    return std::optional<std::pair<Aseba::EventDescription, std::size_t>>(std::pair{m_defs.events[id], id});
}

void aseba_node::get_description() {
    if(m_protocol_version >= 8) {
        request_next_description_fragment();
    } else {
        write_message(std::make_unique<Aseba::GetNodeDescription>(m_id));
    }
}
void aseba_node::handle_description_messages(const Aseba::Message& msg) {
    Aseba::TargetDescription& desc = m_description;
    auto& counter = m_description_message_counter;

    const auto safe_description_update = [](auto&& description, auto& list, uint16_t& counter) {
        if(counter >= list.size())
            return;
        auto it = std::find_if(std::begin(list), std::end(list),
                               [&description](const auto& variable) { return variable.name == description.name; });
        if(it != std::end(list))
            return;
        list[counter++] = std::forward<decltype(description)>(description);
    };

    switch(msg.type) {
        case ASEBA_MESSAGE_DESCRIPTION:
            if(!desc.name.empty()) {
                mLogWarn("Received an Aseba::Description but we already got one");
            }
            desc = static_cast<const Aseba::Description&>(msg);
            break;
        case ASEBA_MESSAGE_NAMED_VARIABLE_DESCRIPTION:
            safe_description_update(static_cast<const Aseba::NamedVariableDescription&>(msg), desc.namedVariables,
                                    counter.variables);
            break;
        case ASEBA_MESSAGE_LOCAL_EVENT_DESCRIPTION:
            safe_description_update(static_cast<const Aseba::LocalEventDescription&>(msg), desc.localEvents,
                                    counter.events);
            break;
        case ASEBA_MESSAGE_NATIVE_FUNCTION_DESCRIPTION:
            safe_description_update(static_cast<const Aseba::NativeFunctionDescription&>(msg), desc.nativeFunctions,
                                    counter.functions);
            break;
        default: return;
    }

    // see request_next_description_fragment
    m_resend_timer.cancel();

    const bool ready = !desc.name.empty() && counter.variables == desc.namedVariables.size() &&
        counter.events == desc.localEvents.size() && counter.functions == desc.nativeFunctions.size();

    if(!ready && m_protocol_version >= 8) {
        request_next_description_fragment();
    }
    if(ready)
        on_description_received();
}

void aseba_node::request_next_description_fragment() {
    auto& counter = m_description_message_counter;
    int16_t fragment = -1;
    if(m_description.protocolVersion > 0) {  // Description received
        fragment = counter.variables + counter.functions + counter.events;
    }
    write_message(std::make_unique<Aseba::GetNodeDescriptionFragment>(fragment, m_id));

    // retrigger a description fragment message in case it's dropped by the wireless key
    m_resend_timer.expires_from_now(boost::posix_time::seconds(1));
    m_resend_timer.async_wait([ptr = weak_from_this()](boost::system::error_code ec) {
        if(ec)
            return;
        auto that = ptr.lock();
        if(!that)
            return;
        that->request_next_description_fragment();
    });
}

}  // namespace mobsya
