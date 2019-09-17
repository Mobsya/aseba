#include "group.h"
#include <aseba/common/utils/utils.h>
#include "error.h"
#include "property.h"
#include "aseba_endpoint.h"
#include "uuid_provider.h"
#include "aseba_property.h"
#include "aesl_parser.h"
#include <range/v3/view/filter.hpp>
#include <range/v3/view/transform.hpp>
#include <range/v3/view/indirect.hpp>
#include <range/v3/algorithm.hpp>

namespace mobsya {

group::group(boost::asio::io_context& context)
    : m_context(context), m_uuid(boost::asio::use_service<uuid_generator>(m_context).generate()) {}


std::shared_ptr<group> group::make_group_for_endpoint(boost::asio::io_context& context,
                                                      std::shared_ptr<aseba_endpoint> initial_ep) {
    auto g = std::make_shared<group>(context);
    g->attach_to_endpoint(initial_ep);
    return g;
}

bool group::has_state() const {
    return m_endpoints.size() > 1 || !m_events_table.empty() || !m_shared_variables.empty();
}

bool group::has_node(const node_id& node) const {
    for(auto&& n : nodes()) {
        if(n->uuid() == node)
            return true;
    }
    return false;
}

node_id group::uuid() const {
    return m_uuid;
}

std::vector<std::shared_ptr<aseba_endpoint>> group::endpoints() const {
    std::vector<std::shared_ptr<aseba_endpoint>> set;
    for(auto&& ptr : m_endpoints) {
        auto ep = ptr.lock();
        if(!ep)
            continue;
        set.push_back(ep);
    }
    return set;
}

void group::attach_to_endpoint(std::shared_ptr<aseba_endpoint> ep) {
    auto eps = endpoints();
    auto it = std::find(eps.begin(), eps.end(), ep);
    if(it == eps.end()) {
        m_endpoints.push_back(ep);
    }

    ep->set_group(this->shared_from_this());
    ep->set_events_table(m_events_table);

    for(auto&& node : ep->nodes()) {
        node->set_status(node->get_status());
    }
    m_variables_changed_signal(shared_from_this(), m_shared_variables);
    m_events_changed_signal(shared_from_this(), m_events_table);
    assign_scratchpads();
}

std::vector<std::shared_ptr<aseba_node>> group::nodes() const {
    std::vector<std::shared_ptr<aseba_node>> nodes;
    for(auto&& e : endpoints()) {
        auto endpoint_nodes = e->nodes();
        nodes.reserve(endpoint_nodes.size());
        std::copy(endpoint_nodes.begin(), endpoint_nodes.end(), std::back_inserter(nodes));
    }
    return nodes;
}

void group::emit_events(const properties_map& map, write_callback&& cb) {

    // call the callback when all endpoints send the events
    // using the deleter of a shared_ptr owning the callback
    const auto deleter = [](write_callback* cb) {
        if(*cb)
            (*cb)({});
        delete cb;
    };
    const auto ptr = std::shared_ptr<write_callback>(new write_callback(std::move(cb)), deleter);

    for(const auto& ep : endpoints()) {
        ep->emit_events(map, [cb = ptr](boost::system::error_code ec) mutable {
            // If one endpoint fails, report the error of that endpoint ( and ignore further errors)
            if(ec && cb && *cb) {
                (*cb)(ec);
                cb.reset();
            }
        });
    }
}

void group::on_event_received(std::shared_ptr<aseba_endpoint> source_ep, const node_id&,
                              const group::properties_map& events, const std::chrono::system_clock::time_point&) {
    // Broadcast the events to other endpoints
    for(const auto& ep : endpoints()) {
        if(ep == source_ep)
            continue;
        ep->emit_events(events, {});
    }
}

boost::system::error_code group::set_shared_variables(const properties_map& map) {
    m_shared_variables = map;
    for(auto&& ep : endpoints()) {
        ep->set_shared_variables(map);
    }
    m_variables_changed_signal(shared_from_this(), m_shared_variables);
    return {};
}


boost::system::error_code group::set_events_table(const events_table& events) {
    m_events_table = events;
    for(auto&& ep : endpoints()) {
        ep->set_events_table(events);
    }
    send_events_table();
    return {};
}

group::events_table group::get_events_table() const {
    return m_events_table;
}

void group::send_events_table() {
    m_events_changed_signal(shared_from_this(), m_events_table);
}

boost::system::error_code group::load_code(std::string_view data, fb::ProgrammingLanguage language) {
    if(language != fb::ProgrammingLanguage::Aesl) {
        return make_error_code(mobsya::error_code::unsupported_language);
    }
    auto aesl = mobsya::load_aesl(data);
    if(!aesl) {
        mLogError("Invalid Aesl");
        return make_error_code(mobsya::error_code::invalid_aesl);
    }
    auto [constants, events, nodes] = aesl->parse_all();
    if(!constants || !events || !nodes) {
        mLogError("Invalid Aesl");
        return make_error_code(mobsya::error_code::invalid_aesl);
    }

    properties_map shared_vars;
    for(const auto& constant : *constants) {
        if(!constant.second.is_integral())
            continue;
        auto v = numeric_cast<int16_t>(property::integral_t(constant.second));
        if(v) {
            shared_vars.emplace(constant.first, v.value());
        }
    }

    events_table table;
    for(const auto& event : *events) {
        if(event.type != event_type::aseba)
            continue;
        table.emplace_back(event.name, event.size);
    }

    for(auto&& s : m_scratchpads) {
        s.deleted = true;
        m_scratchpad_changed_signal(shared_from_this(), s);
    }
    m_scratchpads.clear();

    for(const auto& node : *nodes) {
        auto& s = variant_ns::visit(
            overloaded{
                [this, &node](variant_ns::monostate) -> scratchpad& {
                    return find_or_create_scratch_pad({}, node.name, {});
                },
                [this, &node](node_id id) -> scratchpad& { return find_or_create_scratch_pad(id, node.name, {}); },
                [this, &node](uint16_t id) -> scratchpad& { return find_or_create_scratch_pad({}, node.name, id); }},
            node.id);
        s.text = node.code;
        s.language = fb::ProgrammingLanguage::Aesl;
    }

    set_shared_variables(shared_vars);
    set_events_table(table);
    assign_scratchpads();

    return {};
}  // namespace mobsya

boost::system::error_code group::set_node_scratchpad(node_id id, std::string_view content,
                                                     fb::ProgrammingLanguage language) {
    auto& pad = find_or_create_scratch_pad_for_node(id);
    pad.text = content;
    pad.language = language;
    m_scratchpad_changed_signal(shared_from_this(), pad);
    return {};
}


const std::vector<group::scratchpad>& group::scratchpads() const {
    return m_scratchpads;
}

group::scratchpad& group::create_scratchpad() {
    auto uuid = boost::asio::use_service<uuid_generator>(m_context).generate();
    scratchpad s;
    s.scratchpad_id = uuid;
    s.aseba_id = -1;
    m_scratchpads.push_back(s);
    return m_scratchpads.back();
}

group::scratchpad& group::find_or_create_scratch_pad(std::optional<node_id> preferred_node_id,
                                                     std::optional<std::string_view> name, std::optional<uint16_t> id) {
    auto it =
        std::find_if(m_scratchpads.begin(), m_scratchpads.end(), [preferred_node_id, name, id](auto&& scratchpad) {
            if((preferred_node_id && scratchpad.nodeid == *preferred_node_id) ||
               (preferred_node_id && scratchpad.preferred_node_id == *preferred_node_id) ||
               (id && scratchpad.aseba_id == *id) || (name && scratchpad.name == *name))
                return true;
            return false;
        });
    if(it != m_scratchpads.end()) {
        return *it;
    }
    auto& g = create_scratchpad();
    if(preferred_node_id)
        g.preferred_node_id = *preferred_node_id;
    if(name)
        g.name = *name;
    if(id)
        g.aseba_id = *id;
    return g;
}

group::scratchpad& group::find_or_create_scratch_pad_for_node(node_id id) {
    auto it = std::find_if(m_scratchpads.begin(), m_scratchpads.end(),
                           [id](auto&& scratchpad) { return scratchpad.nodeid == id; });
    if(it != m_scratchpads.end()) {
        return *it;
    }
    auto& g = create_scratchpad();
    g.nodeid = id;
    return g;
}

void group::assign_scratchpads() {
    // Try to match the scratchpads to a node using first the node id, the aseba id and the name.
    // Remaining unmatched scratchpads are assigned to any node available
    std::set<scratchpad*> modified;

    const auto all_nodes = nodes();
    auto connected_nodes = all_nodes | ranges::view::indirect | ranges::view::remove_if([](const auto& node) {
                               return node.get_status() == aseba_node::status::disconnected ||
                                   node.get_status() == aseba_node::status::connected;
                           });

    auto connected_nodes_ids = connected_nodes | ranges::view::transform([](const auto& node) { return node.uuid(); });

    for(scratchpad& s : m_scratchpads) {
        if(ranges::find(connected_nodes_ids, s.nodeid) == ranges::end(connected_nodes_ids)) {
            s.nodeid = {};
            modified.insert(&s);
        }
    }

    auto free_nodes = [&] {
        auto used = m_scratchpads | ranges::view::transform([](const scratchpad& s) { return s.nodeid; }) |
            ranges::view::remove_if([](const node_id& id) { return id.is_nil(); }) | ranges::to_<std::vector>();
        return connected_nodes | ranges::view::remove_if([used](const aseba_node& node) {
                   return ranges::find(used, node.uuid()) != ranges::end(used);
               });
    };

    auto to_assign = [&] {
        return m_scratchpads | ranges::view::remove_if([](const scratchpad& s) { return !s.nodeid.is_nil(); });
    };

    auto assign = [&](auto&& predicate) {
        for(auto&& n : free_nodes()) {
            auto v = to_assign();
            const auto it = ranges::find_if(v, [&n, &predicate](const scratchpad& s) { return predicate(n, s); });
            if(it != ranges::end(v)) {
                it->nodeid = n.uuid();
                modified.insert(&(*it));
            }
        }
    };

    assign([](const aseba_node& n, const scratchpad& s) { return s.nodeid == n.uuid(); });
    assign([](const aseba_node& n, const scratchpad& s) { return s.aseba_id == n.native_id(); });
    assign([](const aseba_node& n, const scratchpad& s) { return s.name == n.friendly_name(); });

    for(auto&& s : to_assign()) {
        auto v = free_nodes();
        if(ranges::empty(v))
            return;
        s.nodeid = ranges::begin(v)->uuid();
        modified.insert(&s);
    }

    for(auto& s : modified) {
        m_scratchpad_changed_signal(shared_from_this(), *s);
    }
}


}  // namespace mobsya