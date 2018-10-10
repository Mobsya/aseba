#include "aesl_parser.h"
#include <aseba/common/consts.h>
#include <boost/uuid/uuid.hpp>
#include "log.h"
#include "utils.h"


namespace mobsya {

aesl_parser::aesl_parser(pugi::xml_document doc) : m_doc(std::move(doc)) {}


aesl_parser::expected<std::map<std::string, property>> aesl_parser::constants() const {
    std::map<std::string, property> properties;
    pugi::xpath_node_set nodes = m_doc.select_nodes("/network/constant");
    for(const auto& node : nodes) {
        std::string n = node.node().attribute("name").as_string();
        auto v = node.node().attribute("value");
        if(n.empty() || v.empty()) {
            return tl::make_unexpected(error::invalid_constant_declaration);
        }
        auto value = lexical_cast<int16_t>(v.as_string());
        if(!value)
            return tl::make_unexpected(error::invalid_constant_type);
        properties.insert_or_assign(n, *value);
    }
    return properties;
}

aesl_parser::expected<std::vector<event>> aesl_parser::global_events() const {
    std::vector<event> events;
    pugi::xpath_node_set nodes = m_doc.select_nodes("/network/event");
    events.reserve(nodes.size());
    for(const auto& node : nodes) {
        auto n = node.node().attribute("name");
        auto s = node.node().attribute("size");
        if(!n || !s) {
            return tl::make_unexpected(error::invalid_event_declaration);
        }
        std::string name = n.as_string();
        auto size = lexical_cast<uint64_t>(s.as_string());
        if(size && name.size() > 0 && *size <= ASEBA_MAX_EVENT_ARG_COUNT) {
            events.emplace_back(name, *size);
            continue;
        }
        // TODO: support font non-int constant
        return tl::make_unexpected(error::invalid_event_type);
    }
    return events;
}

aesl_parser::expected<std::vector<aesl_parser::node_info>> aesl_parser::nodes() const {
    std::vector<node_info> nodes;
    pugi::xpath_node_set xml_nodes = m_doc.select_nodes("/network/node");
    nodes.reserve(xml_nodes.size());

    for(const auto& node : xml_nodes) {
        auto name_attr = node.node().attribute("name");
        auto id_attr = node.node().attribute("nodeId");
        auto txt = node.node().text();
        if(txt.empty()) {
            return tl::make_unexpected(error::invalid_event_type);
        }
        node_info node_info;
        node_info.name = std::optional<std::string>{name_attr.as_string()};
        node_info.code = txt.as_string();
        try {
            auto uuid = boost::uuids::string_generator()(id_attr.as_string());
            if(uuid.version() != boost::uuids::uuid::version_unknown) {
                node_info.id = uuid;
            }
        } catch(...) {
            auto id = lexical_cast<uint16_t>(id_attr.as_llong());
            if(id && *id > 0)
                node_info.id = *id;
        }
        if(id_attr && node_info.id.index() == 0) {
            return tl::make_unexpected(error::invalid_event_type);
        }
        nodes.emplace_back(node_info);
    }
    return nodes;
}


tl::expected<aesl_parser, pugi::xml_parse_status> load_aesl(std::string_view data) {
    pugi::xml_document doc;
    auto res = doc.load_buffer(data.data(), data.size(), pugi::parse_default, pugi::encoding_utf8);
    if(!res) {
        mLogError(res.description());
        return tl::make_unexpected(res.status);
    }
    return aesl_parser(std::move(doc));
}

}  // namespace mobsya