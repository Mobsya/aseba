#pragma once
#include <string_view>
#include <pugixml.hpp>
#include <tl/expected.hpp>
#include <map>
#include "events.h"
#include "property.h"
#include "node_id.h"

namespace mobsya {

class aesl_parser {
public:
    struct node_info {
        std::optional<std::string> name;
        variant_ns::variant<variant_ns::monostate, uint16_t, node_id> id;
        std::string code;
    };
    enum error {
        invalid_constant_declaration,
        invalid_constant_type,
        invalid_event_declaration,
        invalid_event_type,
        invalid_node_declaration,
    };

    template <typename T>
    using expected = tl::expected<T, error>;

    aesl_parser(pugi::xml_document doc);

    expected<std::map<std::string, property>> constants() const;
    expected<std::vector<event>> global_events() const;
    expected<std::vector<node_info>> nodes() const;

private:
    pugi::xml_document m_doc;
};

tl::expected<aesl_parser, pugi::xml_parse_status> load_aesl(std::string_view data);

}  // namespace mobsya