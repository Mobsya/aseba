#pragma once
#include <boost/uuid/uuid.hpp>
#include <aseba/compiler/compiler.h>
#include <boost/system/error_code.hpp>
#include "property.h"
#include "events.h"
#include "aseba_node.h"

namespace mobsya {
class aseba_endpoint;
class group : public std::enable_shared_from_this<group> {
public:
    using properties_map = mobsya::variables_map;
    using events_table = std::vector<mobsya::event>;
    using write_callback = std::function<void(boost::system::error_code)>;


    group(boost::asio::io_context& context);

    bool has_state() const;

    bool has_node(const node_id& node) const;

    node_id uuid() const;

    std::vector<std::shared_ptr<aseba_endpoint>> endpoints() const;

    void attach_to_endpoint(std::shared_ptr<aseba_endpoint> ep);

    boost::system::error_code set_shared_variables(const properties_map& map);
    properties_map shared_variables() const {
        return m_shared_variables;
    }

    void emit_events(const properties_map& map, write_callback&& cb = {});
    boost::system::error_code set_events_table(const events_table& events);
    events_table get_events_table() const;
    void send_events_table();
    void on_event_received(std::shared_ptr<aseba_endpoint> source_ep, const node_id& source_node,
                           const group::properties_map& events);

    template <typename... ConnectionArgs>
    auto connect_to_variables_changes(ConnectionArgs... args) {
        return m_variables_changed_signal.connect(std::forward<ConnectionArgs>(args)...);
    }

    template <typename... ConnectionArgs>
    auto connect_to_events_description_changes(ConnectionArgs... args) {
        return m_events_changed_signal.connect(std::forward<ConnectionArgs>(args)...);
    }

public:
    static std::shared_ptr<group> make_group_for_endpoint(boost::asio::io_context& context,
                                                          std::shared_ptr<aseba_endpoint> initial_ep);

private:
    std::vector<std::shared_ptr<aseba_node>> nodes() const;

    friend class aseba_node;

    boost::asio::io_context& m_context;
    std::vector<std::weak_ptr<aseba_endpoint>> m_endpoints;

    // Because of compat with aseba, m_events_table needs to remain in insertion order
    events_table m_events_table;
    properties_map m_shared_variables;

    boost::uuids::uuid m_uuid;

    using events_signal_t = boost::signals2::signal<void(std::shared_ptr<group>, events_table)>;
    events_signal_t m_events_changed_signal;
    using variables_signal_t = boost::signals2::signal<void(std::shared_ptr<group>, variables_map)>;
    variables_signal_t m_variables_changed_signal;
};

}  // namespace mobsya
