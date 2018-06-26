#pragma once
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <memory>
#include <type_traits>
#include "flatbuffers_message_writer.h"
#include "flatbuffers_message_reader.h"
#include "flatbuffers_messages.h"
#include "aseba_node_registery.h"

#include "log.h"

namespace mobsya {
using tcp = boost::asio::ip::tcp;
namespace websocket = boost::beast::websocket;
using websocket_t = websocket::stream<tcp::socket>;

template <typename Self, typename Socket>
class application_endpoint_base : public std::enable_shared_from_this<application_endpoint_base<Self, Socket>> {
public:
    application_endpoint_base(boost::asio::io_context& ctx) = delete;
    void read_message() = delete;
    void start() = delete;
    void write_message(const flatbuffers::DetachedBuffer& buffer) = delete;
    tcp::socket& tcp_socket() = delete;
};


template <typename Self>
class application_endpoint_base<Self, websocket_t>
    : public std::enable_shared_from_this<application_endpoint_base<Self, websocket_t>> {
public:
    application_endpoint_base(boost::asio::io_context& ctx)
        : m_ctx(ctx), m_socket(tcp::socket(ctx)), m_strand(ctx.get_executor()) {}
    void read_message() {
        auto that = this->shared_from_this();


        auto cb = boost::asio::bind_executor(
            m_strand, [that](boost::system::error_code ec, std::size_t bytes_transferred) mutable {
                if(ec) {
                    mLogError("read_message :{}", ec.message());
                    return;
                }
                std::vector<uint8_t> buf(boost::asio::buffers_begin(that->m_buffer.data()),
                                         boost::asio::buffers_begin(that->m_buffer.data()) + bytes_transferred);
                fb_message_ptr msg(std::move(buf));
                static_cast<Self&>(*that).handle_read(ec, std::move(msg));
                that->m_buffer.consume(bytes_transferred);
            });
        m_socket.async_read(m_buffer, std::move(cb));
    }

    void write_message(const flatbuffers::DetachedBuffer& buffer) {
        auto that = this->shared_from_this();
        auto cb = boost::asio::bind_executor(m_strand, [that](boost::system::error_code ec, std::size_t s) {
            static_cast<Self&>(*that).handle_write(ec);
        });
        m_socket.async_write(boost::asio::buffer(buffer.data(), buffer.size()), std::move(cb));
    }
    void start() {
        m_socket.binary(true);
        auto that = this->shared_from_this();
        auto cb = boost::asio::bind_executor(
            m_strand, [that](boost::system::error_code ec) mutable { static_cast<Self&>(*that).on_initialized(ec); });
        m_socket.async_accept(std::move(cb));
    }

    tcp::socket& tcp_socket() {
        return m_socket.next_layer();
    }

protected:
    boost::asio::io_context& m_ctx;
    boost::asio::strand<boost::asio::io_context::executor_type> m_strand;

private:
    boost::beast::multi_buffer m_buffer;
    websocket_t m_socket;
};


template <typename Self>
class application_endpoint_base<Self, tcp::socket>
    : public std::enable_shared_from_this<application_endpoint_base<Self, tcp::socket>> {
public:
    application_endpoint_base(boost::asio::io_context& ctx)
        : m_ctx(ctx), m_socket(tcp::socket(ctx)), m_strand(ctx.get_executor()) {}
    void read_message() {
        auto that = this->shared_from_this();
        auto cb = boost::asio::bind_executor(m_strand, [that](boost::system::error_code ec, fb_message_ptr msg) {
            static_cast<Self*>(that)->handle_read(ec, std::move(msg));
        });
        mobsya::async_read_flatbuffers_message(m_socket, std::move(cb));
    }

    void write_message(const flatbuffers::DetachedBuffer& buffer) {
        auto cb = boost::asio::bind_executor(
            m_strand, [that = this->shared_from_this()](boost::system::error_code ec, std::size_t s) {
                static_cast<Self&>(*that).handle_write(ec);
            });
        mobsya::async_write_flatbuffer_message(m_socket, buffer, std::move(cb));
    }

    void start() {
        static_cast<Self*>(this)->on_initialized();
    }

    tcp::socket& tcp_socket() {
        return m_socket;
    }

protected:
    boost::asio::io_context& m_ctx;
    boost::asio::strand<boost::asio::io_context::executor_type> m_strand;

private:
    tcp::socket m_socket;
};

template <typename Socket>
class application_endpoint : public application_endpoint_base<application_endpoint<Socket>, Socket>,
                             public node_status_monitor {
public:
    using base = application_endpoint_base<application_endpoint<Socket>, Socket>;
    application_endpoint(boost::asio::io_context& ctx) : base(ctx) {}
    void start() {
        mLogInfo("Starting app endpoint");
        base::start();
    }

    void on_initialized(boost::system::error_code ec = {}) {
        mLogError("on_initialized: {}", ec.message());

        // start listening for incomming messages
        read_message();

        // Subscribe to node change events
        start_node_monitoring(registery());

        // Immediately send a list of nodes
        send_full_node_list();
    }

    void read_message() {
        // boost::asio::post(this->m_strand, boost::bind(&base::read_message, this));
        base::read_message();
    }

    void write_message(flatbuffers::DetachedBuffer&& buffer) {
        m_queue.push_back(std::move(buffer));
        if(m_queue.size() > 1)
            return;

        base::write_message(m_queue.front());
    }


    void handle_read(boost::system::error_code ec, fb_message_ptr&& msg) {
        if(ec)
            return;
        read_message();  // queue the next read early


        mLogError("{} -> {} ", ec.message(), EnumNameAnyMessage(msg.message_type()));
        switch(msg.message_type()) {
            case mobsya::fb::AnyMessage::RequestListOfNodes: send_full_node_list(); break;
            case mobsya::fb::AnyMessage::RequestNodeAsebaVMDescription:
                mLogError("{} -> ", msg.as<fb::RequestNodeAsebaVMDescription>()->node_id());
                send_aseba_vm_description(msg.as<fb::RequestNodeAsebaVMDescription>()->node_id());
                break;
            case mobsya::fb::AnyMessage::LockNode: {
                auto lock_msg = msg.as<fb::LockNode>();
                this->lock_node(lock_msg->node_id(), lock_msg->request_id());
                break;
            }
            case mobsya::fb::AnyMessage::UnlockNode: {
                break;
                auto lock_msg = msg.as<fb::UnlockNode>();
                this->lock_node(lock_msg->node_id(), lock_msg->request_id());
                break;
            }
            default: mLogWarn("Message {} from application unsupported", EnumNameAnyMessage(msg.message_type())); break;
        }
    }

    void handle_write(boost::system::error_code ec) {
        mLogError("{}", ec.message());
        m_queue.erase(m_queue.begin());
        if(!m_queue.empty()) {
            base::write_message(m_queue.front());
        }
    }

    ~application_endpoint() {
        mLogInfo("Stopping app endpoint");

        /* Disconnecting the node monotoring status before unlocking the nodes,
         * otherwise we would receive node status event during destroying the endpoint, leading to a crash */
        disconnect();

        for(auto& p : m_locked_nodes) {
            auto ptr = p.second.lock();
            if(ptr) {
                ptr->unlock(this);
            }
        }
    }

    void node_changed(std::shared_ptr<aseba_node> node, aseba_node_registery::node_id id, aseba_node::status status) {
        mLogInfo("node changed: {}, {}", node->native_id(), node->status_to_string(status));

        flatbuffers::FlatBufferBuilder builder;
        std::vector<flatbuffers::Offset<fb::Node>> nodes;
        nodes.emplace_back(fb::CreateNodeDirect(builder, id, mobsya::fb::NodeStatus(status), fb::NodeType::Thymio2));
        auto vector_offset = builder.CreateVector(nodes);
        auto offset = CreateNodesChanged(builder, vector_offset);
        write_message(wrap_fb(builder, offset));
    }

private:
    void send_full_node_list() {
        flatbuffers::FlatBufferBuilder builder;
        std::vector<flatbuffers::Offset<fb::Node>> nodes;
        auto map = registery().nodes();
        for(auto& node : map) {
            const auto ptr = node.second.lock();
            if(!ptr)
                continue;
            nodes.emplace_back(fb::CreateNodeDirect(builder, node.first, mobsya::fb::NodeStatus(ptr->get_status()),
                                                    fb::NodeType::Thymio2));
        }
        auto vector_offset = builder.CreateVector(nodes);
        auto offset = CreateNodesChanged(builder, vector_offset);
        write_message(wrap_fb(builder, offset));
    }

    void send_aseba_vm_description(aseba_node_registery::node_id id) {
        auto node = registery().node_from_id(id);
        if(!node) {
            // error ?
            return;
        }
        write_message(serialize_aseba_vm_description(*node, id));
    }

    void lock_node(aseba_node_registery::node_id id, uint32_t request_id) {
        auto node = registery().node_from_id(id);
        if(!node) {
            write_message(create_error_response(request_id, fb::ErrorType::unknown_node));
            return;
        }
        auto res = node->lock(this);
        if(!res) {
            write_message(create_error_response(request_id, fb::ErrorType::node_busy));
        } else {
            write_message(create_lock_response(request_id, id));
            m_locked_nodes.insert(std::pair{id, std::weak_ptr<aseba_node>{node}});
        }
    }

    void unlock_node(aseba_node_registery::node_id id, uint32_t request_id) {

        auto it = m_locked_nodes.find(id);
        std::shared_ptr<aseba_node> node;
        if(it != std::end(m_locked_nodes)) {
            node = it->second->lock();
            m_locked_nodes.erase(it);
        }

        if(!node) {
            write_message(create_error_response(request_id, fb::ErrorType::unknown_node));
            return;
        }
        if(!node->unlock(this)) {
            write_message(create_error_response(request_id, fb::ErrorType::node_busy));
        }
    }

    aseba_node_registery& registery() {
        return boost::asio::use_service<aseba_node_registery>(this->m_ctx);
    }

    std::vector<flatbuffers::DetachedBuffer> m_queue;
    std::unordered_map<aseba_node_registery::node_id, std::weak_ptr<aseba_node>> m_locked_nodes;
};


}  // namespace mobsya
