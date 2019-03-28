#pragma once

#include "error.h"
#include "property.h"

namespace mobsya {
inline tl::expected<std::vector<int16_t>, boost::system::error_code> to_aseba_variable(const property& p,
                                                                                       uint16_t size) {
    if(p.is_null() && size == 0) {
        return {};
    }
    if(size == 1 && p.is_integral()) {
        property::integral_t v(p);
        auto n = numeric_cast<int16_t>(v);
        if(!n)
            return make_unexpected(error_code::incompatible_variable_type);
        return std::vector({*n});
    }
    if(p.is_array() && size == p.size()) {
        std::vector<int16_t> vars;
        vars.reserve(p.size());
        for(std::size_t i = 0; i < p.size(); i++) {
            auto e = p[i];
            if(!e.is_integral()) {
                return make_unexpected(error_code::incompatible_variable_type);
            }
            auto v = property::integral_t(e);
            auto n = numeric_cast<int16_t>(v);
            if(!n)
                return make_unexpected(error_code::incompatible_variable_type);
            vars.push_back(*n);
        }
        return vars;
    }
    return make_unexpected(error_code::incompatible_variable_type);
}

namespace detail {
    template <typename Rng>
    static auto aseba_variable_from_range(Rng&& rng) {
        if(rng.size() == 0)
            return property();
        return property(property::list::from_range(std::forward<Rng>(rng)));
    }
}  // namespace detail


}  // namespace mobsya
