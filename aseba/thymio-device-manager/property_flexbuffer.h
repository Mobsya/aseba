#pragma once
#include "property.h"
#include "expected.hpp"
#include <flatbuffers/flexbuffers.h>

namespace mobsya {
void property_to_flexbuffer(const property& p, flexbuffers::Builder& b);
nonstd::expected<property, std::errc> flexbuffer_to_property(const flexbuffers::Reference& r);

}  // namespace mobsya