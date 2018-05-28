#pragma once
#include <aseba/flatbuffers/thymio_generated.h>

namespace mobsya {

flatbuffers::DetachedBuffer create_nodes_list_request() {
    flatbuffers::FlatBufferBuilder fb;
    auto errOff = mobsya::fb::CreateRequestListOfNodes(fb);
    auto rootOffset = mobsya::fb::CreateMessage(
        fb, mobsya::fb::AnyMessageTraits<mobsya::fb::RequestListOfNodes>::enum_value, errOff.Union());
    fb.FinishSizePrefixed(rootOffset);
    return fb.ReleaseBufferPointer();
}

}  // namespace mobsya
