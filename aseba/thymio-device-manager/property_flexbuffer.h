#pragma once
#include "property.h"
#include <flatbuffers/flexbuffers.h>

namespace mobsya {
void property_to_flexbuffer(const property& p, flexbuffers::Builder& b);

}  // namespace mobsya