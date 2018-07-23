#pragma once
#include <memory>
#include <mutex>
#include "aseba/common/msg/msg.h"
#include "node_id.h"
#include <boost/asio/io_context.hpp>
#include <atomic>


namespace mobsya {
class aseba_endpoint;

class aseba_node : public std::enable_shared_from_this<aseba_node> {
public:
    // aseba_node::status is exposed through zero conf & protocol : needs to be stable
    enum class status { connected = 1, available = 2, busy = 3, ready = 4, disconnected = 5 };
    using node_id_t = uint16_t;

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

    std::string friendly_name() const;
    void set_friendly_name(const std::string& str);

    Aseba::TargetDescription vm_description() const {
        std::unique_lock<std::mutex> _(m_node_mutex);
        return m_description;
    }

    // Write n messages to the enpoint owning that node, then invoke cb when all message have been written
    void write_messages(std::vector<std::shared_ptr<Aseba::Message>>&& message, write_callback&& cb = {});
    // Write a message to the enpoint owning that node, then invoke cb
    void write_message(std::shared_ptr<Aseba::Message> message, write_callback&& cb = {});

    // Compile a program and send it to the node, invoking cb once the assossiated message is written out
    // If the code can not be compiled, returns false without invoking cb
    bool send_aseba_program(const std::string& program, write_callback&& cb = {});
    void run_aseba_program(write_callback&& cb = {});

    bool lock(void* app);
    bool unlock(void* app);

private:
    friend class aseba_endpoint;
    void set_status(status);

    // Must be called before destructor !
    void disconnect();
    void on_message(const Aseba::Message& msg);
    void on_description(Aseba::TargetDescription description);
    void on_device_info(const Aseba::ThymioDeviceInfo& info);

    node_id_t m_id;
    node_id m_uuid;
    std::string m_friendly_name;
    std::atomic<status> m_status;
    std::atomic<void*> m_connected_app;
    std::weak_ptr<mobsya::aseba_endpoint> m_endpoint;
    mutable std::mutex m_node_mutex;
    Aseba::TargetDescription m_description;
    boost::asio::io_context& m_io_ctx;
};


}  // namespace mobsya
