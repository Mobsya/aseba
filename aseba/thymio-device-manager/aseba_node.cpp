#include "aseba_node.h"
#include "aseba_endpoint.h"
#include "aseba_node_registery.h"
#include <aseba/common/utils/utils.h>
#include <aseba/compiler/compiler.h>
#include <fmt/format.h>
#include "aesl_parser.h"
#include "group.h"
#include "aseba_property.h"

namespace mobsya {

static const uint32_t MAX_FRIENDLY_NAME_SIZE = 30;

const std::string& aseba_node::status_to_string(aseba_node::status s) {
    static std::array<std::string, 6> strs = {"connected", "available", "busy", "ready", "disconnected", "upgrading"};
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
    , m_firmware_version(0)
    , m_available_firmware_version(0)
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


std::shared_ptr<mobsya::aseba_endpoint> aseba_node::endpoint() const {
    return m_endpoint.lock();
}

void aseba_node::disconnect() {
    cancel_pending_step_request();
    cancel_pending_breakpoint_request();
    set_status(status::disconnected);
}

aseba_node::~aseba_node() {
	mLogInfo("Destroying node id={}", uuid());
    cancel_pending_step_request();
    cancel_pending_breakpoint_request();
    if(m_status.load() != status::disconnected) {
        mLogWarn("Node destroyed before being disconnected id={}", uuid());
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
    }
}

std::shared_ptr<mobsya::group> aseba_node::group() const {
    auto ep = m_endpoint.lock();
    return ep ? ep->group() : std::shared_ptr<mobsya::group>{};
}

void aseba_node::set_status(status s) {
    m_status = s;
    auto& registery = boost::asio::use_service<aseba_node_registery>(m_io_ctx);
    switch(s) {
        case status::connected: registery.add_node(shared_from_this()); break;
        case status::disconnected: registery.remove_node(shared_from_this()); break;
        default: registery.set_node_status(shared_from_this(), s); break;
    }
    // When the status of a node change, reassign the scratchpad of the associated group
    // because set_status can be called while the endpoint is being destroyed,
    // we need to postpone the call
    boost::asio::post(m_io_ctx, [g = group()]() {
        if(g)
            g->assign_scratchpads();
    });
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
    mLogDebug("Unlocking node id={}", uuid());
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
    Aseba::CommonDefinitions defs = endpoint()->aseba_compiler_definitions();
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

/*
Non inplace Byte swap for being compliant with Endianess
* optimization can arise using pointer and inplace memory
*/
uint16_t swapByteOrder(uint16_t us) { 
    uint16_t new_us = (us >> 8) | (us << 8); 
    return new_us;
}

/* **********
* The function compiles the code and create the proper header for the .abo file 
* such that the Thymio firmware can de-serialize it correctly 
* fileformat is described here http://wiki.thymio.org/asebaspecifications001 
********** */ 
std::vector<uint16_t> aseba_node::compile_and_save(fb::ProgrammingLanguage language, const std::string& program,
                                          compilation_callback&& cb) {
    m_breakpoints.clear();
    cancel_pending_step_request();
    cancel_pending_breakpoint_request();
    Aseba::Compiler compiler;
    Aseba::CommonDefinitions defs = endpoint()->aseba_compiler_definitions();

    compiler.setTargetDescription(&m_description);
    compiler.setCommonDefinitions(&defs);
    auto result = do_compile_program(compiler, defs, language, program, m_bytecode);

    std::vector<uint16_t> data_buff = std::vector<uint16_t>(m_bytecode.begin(), m_bytecode.end());

    if(!result) {
        cb(result.error(), {});
        return data_buff;
    }

    std::vector<uint16_t> fin_data_buff;

    fin_data_buff.push_back((uint16_t)'A');
    fin_data_buff.push_back((uint16_t)'B');
    fin_data_buff.push_back((uint16_t)'O');
    fin_data_buff.push_back((uint16_t)'\0');

    fin_data_buff.push_back((uint16_t)0);
    fin_data_buff.push_back((uint16_t)swapByteOrder(m_description.protocolVersion));
    fin_data_buff.push_back((uint16_t)swapByteOrder(8));

    auto it = std::find_if(m_variables.begin(), m_variables.end(),
                        [](const aseba_vm_variable& var) { return var.name == "_fwversion"; });
    if(it != m_variables.end()) {
        fin_data_buff.push_back((uint16_t)swapByteOrder(((*it).value[0])));
    }

    fin_data_buff.push_back((uint16_t)swapByteOrder(1));
    fin_data_buff.push_back((uint16_t)swapByteOrder(Aseba::crcXModem(0,m_description.name)));            
    fin_data_buff.push_back((uint16_t)swapByteOrder(m_description.crc()));
    fin_data_buff.push_back((uint16_t)swapByteOrder(data_buff.size()));

    uint16_t overall_crc(0);
    for(int k = 0; k < data_buff.size(); k++){
        fin_data_buff.push_back(swapByteOrder(data_buff[k]));
        overall_crc = Aseba::crcXModem(overall_crc, data_buff[k]);
    }

    fin_data_buff.push_back((uint16_t)swapByteOrder(overall_crc));

    return fin_data_buff;
}


void aseba_node::compile_and_send_program(fb::ProgrammingLanguage language, const std::string& program,
                                          compilation_callback&& cb) {
    m_breakpoints.clear();
    cancel_pending_step_request();
    cancel_pending_breakpoint_request();
    Aseba::Compiler compiler;
    Aseba::CommonDefinitions defs = endpoint()->aseba_compiler_definitions();

    compiler.setTargetDescription(&m_description);
    compiler.setCommonDefinitions(&defs);
    auto result = do_compile_program(compiler, defs, language, program, m_bytecode);
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

    m_variables_changed_signal(shared_from_this(), this->variables(), std::chrono::system_clock::now());
}

tl::expected<aseba_node::compilation_result, boost::system::error_code>
aseba_node::do_compile_program(Aseba::Compiler& compiler, Aseba::CommonDefinitions&, fb::ProgrammingLanguage language,
                               const std::string& program, Aseba::BytecodeVector& bytecode) {

    if(language == fb::ProgrammingLanguage::Aesl) {
        return tl::make_unexpected(make_error_code(mobsya::error_code::unsupported_language));
    }

    std::wstring code = Aseba::UTF8ToWString(program);
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

void aseba_node::compile_and_send_aseba_command(const std::string& program) {
    Aseba::Compiler compiler;
    Aseba::CommonDefinitions defs = endpoint()->aseba_compiler_definitions();

    compiler.setTargetDescription(&m_description);
    compiler.setCommonDefinitions(&defs);

    std::wstring code = Aseba::UTF8ToWString(program);
    compilation_result result;
    std::wistringstream is(code);
    Aseba::Error error;
    m_bytecode.clear();
    unsigned allocatedVariablesCount;

    compiler.compile(is, m_bytecode, allocatedVariablesCount, error);

    std::vector<std::shared_ptr<Aseba::Message>> messages;
    Aseba::sendBytecode(messages, native_id(), std::vector<uint16_t>(m_bytecode.begin(), m_bytecode.end()));

    write_messages(std::move(messages));
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
            compile_and_send_aseba_command(R"(
                                         motor.left.target = 0
                                         motor.right.target = 0 )");
            write_message(std::make_shared<Aseba::Run>(native_id()), std::move(cb));
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
                return int(i);
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

aseba_node::vm_execution_state aseba_node::execution_state() const {
    return {m_vm_state.state, m_vm_state.line, fb::VMExecutionError::NoError, {}};
}

void aseba_node::on_event_received(const std::unordered_map<std::string, property>& events,
                                   const std::chrono::system_clock::time_point& timestamp) {
    m_events_signal(shared_from_this(), events, timestamp);
}


boost::system::error_code aseba_node::set_node_variables(const variables_map& map, write_callback&& cb) {
    std::vector<std::shared_ptr<Aseba::Message>> messages;
    messages.reserve(map.size());
    variables_map modified;
    for(auto&& var : map) {
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

    write_messages(std::move(messages), std::move(cb));
    if(!modified.empty()) {
        m_variables_changed_signal(shared_from_this(), modified, std::chrono::system_clock::now());
    }
    return {};
}

void aseba_node::rename(const std::string& newName) {
    set_friendly_name(newName);
    write_message(std::make_shared<Aseba::GetDeviceInfo>(native_id(), DEVICE_INFO_NAME));
}

void aseba_node::on_description_received() {
    mLogInfo("Got description for {} id={} [{} variables, {} functions, {} events - protocol {}]", native_id(), uuid(),
             m_description.namedVariables.size(), m_description.nativeFunctions.size(),
             m_description.localEvents.size(), m_description.protocolVersion);
    unsigned count;
    reset_known_variables(m_description.getVariablesMap(count));
    schedule_variables_update();
    schedule_execution_state_update();

    auto it = std::find_if(m_variables.begin(), m_variables.end(),
                           [](const aseba_vm_variable& var) { return var.name == "_fwversion"; });
    if(it != m_variables.end()) {
        write_message(std::make_shared<Aseba::GetVariables>(native_id(), it->start, it->size));
    }

    if(m_description.protocolVersion >= 6 &&
       (type() == aseba_node::node_type::Thymio2 || (type() == aseba_node::node_type::Thymio2Wireless))) {
        request_device_info();
        return;
    }
    set_status(status::available);
    // this->upgrade_firmware();
}


void aseba_node::request_device_info() {
    if(m_uuid_received || m_description.protocolVersion < 6)
        return;
    write_message(std::make_shared<Aseba::GetDeviceInfo>(native_id(), DEVICE_INFO_NAME));
    if(m_description.protocolVersion >= 9) {
        write_message(std::make_shared<Aseba::GetDeviceInfo>(native_id(), DEVICE_INFO_THYMIO2_RF_SETTINGS));
    }
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

    // Set all the variables to null
    variables_map removed;
    for(auto&& var : m_variables) {
        removed.emplace(var.name, property{});
    }

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
            // don't mark the variables we keep as removed
            removed.erase(name);
        }
    }

    // Signal the removed variables to watching applications
    m_variables_changed_signal(shared_from_this(), removed, std::chrono::system_clock::now());

    // Ask the device for all variables
    // This will sync up the value of non-removed variables
    m_resend_all_variables = true;
}

void aseba_node::on_variables_message(const Aseba::Variables& msg) {
    variables_map changed;
    set_variables(msg.start, msg.variables, changed);
    m_variables_changed_signal(shared_from_this(), changed, std::chrono::system_clock::now());
    schedule_variables_update();
}

void aseba_node::on_variables_message(const Aseba::ChangedVariables& msg) {
    variables_map changed;
    for(const auto& area : msg.variables) {
        set_variables(area.start, area.variables, changed);
    }
    m_variables_changed_signal(shared_from_this(), changed, std::chrono::system_clock::now());
    schedule_variables_update();
}

void aseba_node::set_variables(uint16_t start, const std::vector<int16_t>& data, variables_map& vars) {

    auto data_it = std::begin(data);
    auto it = m_variables.begin();
	//mLogTrace("Set variable loop id={}", uuid());
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
            mLogTrace("For node id={}, variable changed {} : {}", uuid(), var.name, detail::aseba_variable_from_range(var.value));
            if(var.name == "_fwversion" && m_firmware_version != var.value[0]) {
                m_firmware_version = var.value[0];
                set_status(m_status);
            }
        }
        data_it = data_it + count;
        start += uint16_t(count);
    }
}


variables_map aseba_node::variables() const {
    variables_map map;
    map.reserve(m_variables.size());
    for(auto& var : m_variables) {
        map.emplace(var.name, detail::aseba_variable_from_range(var.value));
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
    mLogTrace("Got info for {} id={} [{} : {}]", native_id(), uuid(), info.info, info.data.size());
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
        mLogInfo("Persistent uuid for {} id={} is now {} ", native_id(), uuid(), boost::uuids::to_string(m_uuid));
        auto& registery = boost::asio::use_service<aseba_node_registery>(m_io_ctx);
        registery.handle_node_uuid_change(shared_from_this());
        set_status(status::available);

    } else if(info.info == DEVICE_INFO_NAME) {
        m_friendly_name.clear();
        m_friendly_name.reserve(info.data.size());
        std::copy(info.data.begin(), info.data.end(), std::back_inserter(m_friendly_name));
        set_status(m_status);
         mLogInfo("Persistent name for {} id={} is now \"{}\"", native_id(), uuid(), friendly_name());
    } else if(info.info == DEVICE_INFO_THYMIO2_RF_SETTINGS) {
        if(info.data.size() != 3 * sizeof(uint16_t))
            return;
        const auto d = (uint16_t*)info.data.data();
        m_th2_rfid_settings = {bswap16(d[0]), bswap16(d[1]), bswap16(d[2])};
        mLogInfo("node {} : net node={:x}, network={}, channel={}", native_id(), m_th2_rfid_settings.node_id,
                 m_th2_rfid_settings.network_id, m_th2_rfid_settings.channel);
    }
}

bool aseba_node::set_rf_settings(uint16_t network, uint16_t node, uint8_t channel) {
    if(m_description.protocolVersion < 9)
        return false;
    if(node == 0)
        node = native_id();
    std::vector<uint8_t> data;
    data.resize(6);
    (uint16_t&)(*(data.data())) = bswap16(network);
    (uint16_t&)(*(data.data() + 2)) = bswap16(node);
    (uint16_t&)(*(data.data() + 4)) = bswap16(channel);

    auto ep = endpoint();

    write_message(std::make_shared<Aseba::SetDeviceInfo>(native_id(), DEVICE_INFO_THYMIO2_RF_SETTINGS, data));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    write_message(std::make_shared<Aseba::Reboot>(native_id()));
    write_message(std::make_shared<Aseba::Reboot>(native_id()));
    return true;
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

int aseba_node::firwmware_version() const {
    return m_firmware_version;
}
int aseba_node::available_firwmware_version() const {
    return m_available_firmware_version;
}

void aseba_node::set_available_firmware_version(int version) {
    m_available_firmware_version = version;
}

bool aseba_node::upgrade_firmware(
    std::function<void(boost::system::error_code ec, double progress, bool complete)> cb) {
    if(is_wirelessly_connected() || m_status != aseba_node::status::available)
        return false;
    set_status(status::upgrading);
    if(auto ptr = m_endpoint.lock())
        return ptr->upgrade_firmware(m_id, cb);
    return false;
}

bool aseba_node::can_be_renamed() const {
    auto ep = m_endpoint.lock();
    return ep && ep->type() == aseba_endpoint::endpoint_type::thymio && m_description.protocolVersion >= 6;
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
                mLogWarn("For node id={}, received an Aseba::Description but we already got one", uuid());
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
