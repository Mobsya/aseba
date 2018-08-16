#include "aseba_node.h"
#include "aseba_endpoint.h"
#include "aseba_node_registery.h"
#include <aseba/common/utils/utils.h>
#include <aseba/compiler/compiler.h>
#include <fmt/format.h>
#include <fmt/ranges.h>

namespace mobsya {

const std::string& aseba_node::status_to_string(aseba_node::status s) {
    static std::array<std::string, 5> strs = {"connected", "available", "busy", "ready", "disconnected"};
    int i = int(s) - 1;
    if(i < 0 && i >= int(status::connected))
        return strs[4];
    return strs[i];
}


aseba_node::aseba_node(boost::asio::io_context& ctx, node_id_t id, std::weak_ptr<mobsya::aseba_endpoint> endpoint)
    : m_id(id)
    , m_uuid(boost::uuids::nil_uuid())
    , m_status(status::disconnected)
    , m_connected_app(nullptr)
    , m_endpoint(std::move(endpoint))
    , m_io_ctx(ctx) {}

std::shared_ptr<aseba_node> aseba_node::create(boost::asio::io_context& ctx, node_id_t id,
                                               std::weak_ptr<mobsya::aseba_endpoint> endpoint) {
    auto node = std::make_shared<aseba_node>(ctx, id, std::move(endpoint));
    node->set_status(status::connected);
    return node;
}


void aseba_node::disconnect() {
    set_status(status::disconnected);
}

aseba_node::~aseba_node() {
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
        default: break;
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
    std::unique_lock<std::mutex> _(m_node_mutex);
    if(m_connected_app == app) {
        return true;
    }
    if(m_connected_app != nullptr || m_status != status::available) {
        return false;
    }
    m_connected_app = app;
    mLogDebug("Locking node");
    set_status(status::busy);
    return true;
}

bool aseba_node::unlock(void* app) {
    std::unique_lock<std::mutex> _(m_node_mutex);
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
    std::unique_lock<std::mutex> _(m_node_mutex);  // Probably not necessary ?
    auto endpoint = m_endpoint.lock();
    if(!endpoint) {
        return;
    }
    endpoint->write_messages(std::move(messages), std::move(cb));
}

bool aseba_node::send_aseba_program(const std::string& program, write_callback&& cb) {
    std::unique_lock<std::mutex> _(m_node_mutex);

    Aseba::Compiler compiler;
    Aseba::CommonDefinitions defs;
    compiler.setTargetDescription(&m_description);
    compiler.setCommonDefinitions(&defs);

    auto wprogram = Aseba::UTF8ToWString(program);
    std::wistringstream is(wprogram);
    Aseba::Error error;
    Aseba::BytecodeVector bytecode;
    unsigned allocatedVariablesCount;
    bool result = compiler.compile(is, bytecode, allocatedVariablesCount, error);
    if(!result) {
        mLogWarn("Compilation failed on node {} : {}", m_id, Aseba::WStringToUTF8(error.message));
        return false;
    }

    m_node_mutex.unlock();

    std::vector<std::shared_ptr<Aseba::Message>> messages;
    Aseba::sendBytecode(messages, native_id(), std::vector<uint16_t>(bytecode.begin(), bytecode.end()));
    write_messages(std::move(messages), std::move(cb));
    return true;
}

void aseba_node::run_aseba_program(write_callback&& cb) {
    write_message(std::make_shared<Aseba::Run>(native_id()), std::move(cb));
}

void aseba_node::rename(const std::string&) {
    set_status(m_status);
}

void aseba_node::on_description(Aseba::TargetDescription description) {
    mLogInfo("Got description for {} [{} variables, {} functions, {} events - protocol {}]", native_id(),
             description.namedVariables.size(), description.nativeFunctions.size(), description.localEvents.size(),
             description.protocolVersion);
    {
        std::unique_lock<std::mutex> _(m_node_mutex);
        m_description = std::move(description);
    }
    if(description.protocolVersion >= 6 &&
       (type() == aseba_node::node_type::Thymio2 || (type() == aseba_node::node_type::Thymio2Wireless))) {
        // set_friendly_name("The merovingian");
        write_message(std::make_shared<Aseba::GetDeviceInfo>(native_id(), DEVICE_INFO_NAME));
        write_message(std::make_shared<Aseba::GetDeviceInfo>(native_id(), DEVICE_INFO_UUID));
        return;
    }

    set_status(status::available);
}

void aseba_node::on_device_info(const Aseba::DeviceInfo& info) {
    mLogTrace("Got info for {} [{} : {}]", native_id(), info.info, info.data.size());
    if(info.info == DEVICE_INFO_UUID) {
        if(info.data.size() == 16) {
            std::copy(info.data.begin(), info.data.end(), m_uuid.begin());
        }
        std::unique_lock<std::mutex> lock(m_node_mutex);
        if(m_uuid.is_nil()) {
            m_uuid = boost::uuids::random_generator()();
            std::vector<uint8_t> data;
            std::copy(m_uuid.begin(), m_uuid.end(), std::back_inserter(data));
            lock.unlock();
            write_message(std::make_shared<Aseba::SetDeviceInfo>(native_id(), DEVICE_INFO_UUID, data));
        }
        mLogInfo("Persistent uuid for {} is now {} ", native_id(), boost::uuids::to_string(m_uuid));
        auto& registery = boost::asio::use_service<aseba_node_registery>(m_io_ctx);
        registery.set_node_uuid(shared_from_this(), m_uuid);
        set_status(status::available);

    } else if(info.info == DEVICE_INFO_NAME) {
        std::unique_lock<std::mutex> _(m_node_mutex);
        m_friendly_name.clear();
        m_friendly_name.reserve(info.data.size());
        std::copy(info.data.begin(), info.data.end(), std::back_inserter(m_friendly_name));
        if(!m_friendly_name.empty())
            mLogInfo("Persistent name for {} is now \"{}\"", native_id(), m_friendly_name);
    }
}

aseba_node::node_type aseba_node::type() const {
    std::unique_lock<std::mutex> _(m_node_mutex);
    auto ep = m_endpoint.lock();
    if(!ep) {
        return aseba_node::node_type::UnknownType;
    }
    switch(ep->type()) {
        case aseba_endpoint::endpoint_type::thymio:
            return is_wirelessly_connected() ? node_type::Thymio2Wireless : node_type::Thymio2;
        case aseba_endpoint::endpoint_type::simulated_tymio: return node_type::SimulatedThymio2;
        case aseba_endpoint::endpoint_type::simulated_dummy_node: return node_type::DummyNode;
        default: break;
    }
    return node_type::UnknownType;
}

std::string aseba_node::friendly_name() const {
    std::unique_lock<std::mutex> _(m_node_mutex);
    if(m_friendly_name.empty()) {
        auto ep = m_endpoint.lock();
        if(ep) {
            return ep->endpoint_name();
        }
    }
    return m_friendly_name;
}

void aseba_node::set_friendly_name(const std::string& str) {
    std::vector<uint8_t> data;
    data.reserve(str.size());
    std::copy(str.begin(), str.end(), std::back_inserter(data));
    write_message(std::make_shared<Aseba::SetDeviceInfo>(native_id(), DEVICE_INFO_NAME, data));
    std::unique_lock<std::mutex> _(m_node_mutex);
    m_friendly_name = str;
}

}  // namespace mobsya
