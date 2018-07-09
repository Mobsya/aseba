#include "aseba_node.h"
#include "aseba_endpoint.h"
#include "aseba_node_registery.h"
#include <aseba/common/utils/utils.h>
#include <aseba/compiler/compiler.h>

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


void aseba_node::on_message(const Aseba::Message& msg) {
    /*switch(msg.type) {
        case ASEBA_MESSAGE_NODE_PRESENT: {
            std::unique_lock<std::mutex> _(m_node_mutex);
            if(m_description.name.empty()) {
                auto locked = m_endpoint.lock();
                if(locked) {
                    locked->read_aseba_node_description(m_id);
                }
            }
            break;
        }
        default: break;
    }*/
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

void aseba_node::on_description(Aseba::TargetDescription description) {
    {
        std::unique_lock<std::mutex> _(m_node_mutex);
        m_description = description;
    }
    set_status(status::available);
}

}  // namespace mobsya
