#pragma once
#include "property.h"
#include <tl/expected.hpp>
#include <flatbuffers/flexbuffers.h>
#include "error.h"

namespace mobsya {
void property_to_flexbuffer(const property& p, flexbuffers::Builder& b);
tl::expected<property, error_code> flexbuffer_to_property(const flexbuffers::Reference& r);

}  // namespace mobsya