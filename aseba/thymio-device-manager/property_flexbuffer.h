#include "property.h"
#include <flatbuffers/flexbuffers.h>

namespace mobsya {

namespace detail {
    void serialize_to_flexbuffer(const property& p, flexbuffers::Builder& b) {
        std::visit(detail::overloaded{[&b](const std::monostate&) { b.Null(); },
                                      [&b](const typename property::bool_t& e) { b.Bool(e); },
                                      [&b](const typename property::integral_t& e) { b.Int(e); },
                                      [&b](const typename property::floating_t& e) { b.Double(e); },
                                      [&b](const typename property::string_t& e) { b.String(e); },
                                      [&b](const typename property::array_t& e) {
                                          auto start = b.StartVector();
                                          for(auto&& v : e) {
                                              serialize_to_flexbuffer(v, b);
                                          }
                                          b.EndVector(start, false, false);
                                      },
                                      [&b](const typename property::object_t& e) {
                                          auto start = b.StartMap();
                                          for(auto&& v : e) {
                                              b.Key(v.first);
                                              serialize_to_flexbuffer(v.second, b);
                                          }
                                          b.EndMap(start);
                                      }},
                   p.value);
    }
}  // namespace detail

void property_to_flexbuffer(const property& p, flexbuffers::Builder& b) {
    detail::serialize_to_flexbuffer(p, b);
    b.Finish();
}

}  // namespace mobsya