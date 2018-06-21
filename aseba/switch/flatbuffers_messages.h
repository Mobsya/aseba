#pragma once
#include <aseba/flatbuffers/thymio_generated.h>

namespace mobsya {

template <typename MessageType>
flatbuffers::DetachedBuffer wrap_fb(flatbuffers::FlatBufferBuilder& fb,
                                    const flatbuffers::Offset<MessageType>& offset) {
    auto rootOffset =
        mobsya::fb::CreateMessage(fb, mobsya::fb::AnyMessageTraits<MessageType>::enum_value, offset.Union());
    fb.Finish(rootOffset);
    auto x = fb.ReleaseBufferPointer();
    return x;
}

flatbuffers::DetachedBuffer create_nodes_list_request() {
    flatbuffers::FlatBufferBuilder fb;
    auto errOff = mobsya::fb::CreateRequestListOfNodes(fb);
    return wrap_fb(fb, errOff);
}


//
//

}  // namespace mobsya
