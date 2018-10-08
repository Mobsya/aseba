#include "property_flexbuffer.h"
#include "property.h"
#include <flatbuffers/flexbuffers.h>

namespace mobsya {

namespace detail {
    void serialize_to_flexbuffer(const property& p, flexbuffers::Builder& b) {
        variant_ns::visit(detail::overloaded{[&b](const variant_ns::monostate&) { b.Null(); },
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

tl::expected<property, error_code> flexbuffer_to_property(const flexbuffers::Reference& r) {
    if(r.IsIntOrUint()) {
        return property{r.As<property::integral_t>()};
    }
    if(r.IsFloat()) {
        return property{r.As<property::floating_t>()};
    }
    if(r.IsBool()) {
        return property{r.AsBool()};
    }
    if(r.IsNull()) {
        return property{};
    }
    if(r.IsString()) {
        return property{r.AsString().c_str()};
    }
    if(r.IsVector()) {
        auto v = r.AsVector();
        property::array_t l;
        l.reserve(v.size());
        for(size_t i = 0; i < v.size(); i++) {
            auto p = flexbuffer_to_property(v[i]);
            if(!p) {
                return tl::make_unexpected(p.error());
            }
            l.push_back(p.value());
        }
        return l;
    }
    if(r.IsMap()) {
        auto m = r.AsMap();
        auto keys = m.Keys();
        property::object_t o;
        for(size_t i = 0; i < keys.size(); i++) {
            auto k = keys[i].AsKey();
            if(!k)
                return tl::make_unexpected(error_code::invalid_object);
            auto p = flexbuffer_to_property(m[k]);
            if(!p) {
                return tl::make_unexpected(p.error());
            }
            o.insert_or_assign(k, p.value());
        }
        return o;
    }
    return tl::make_unexpected(error_code::invalid_object);
}

}  // namespace mobsya