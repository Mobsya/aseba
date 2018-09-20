#pragma once
#include <aseba/common/utils/utils.h>
#include <aseba/flatbuffers/thymio_generated.h>
#include <flatbuffers/flatbuffers.h>
#include "aseba_node.h"
#include "aseba_node_registery.h"
#include <aseba/flatbuffers/fb_message_ptr.h>

namespace mobsya {


flatbuffers::DetachedBuffer create_nodes_list_request() {
    flatbuffers::FlatBufferBuilder fb;
    auto offset = mobsya::fb::CreateRequestListOfNodes(fb);
    return wrap_fb(fb, offset);
}

flatbuffers::DetachedBuffer create_error_response(uint32_t request_id, fb::ErrorType error) {
    flatbuffers::FlatBufferBuilder fb;
    auto offset = mobsya::fb::CreateError(fb, request_id, error);
    return wrap_fb(fb, offset);
}

flatbuffers::DetachedBuffer create_compilation_error_response(uint32_t request_id) {
    flatbuffers::FlatBufferBuilder fb;
    auto offset = mobsya::fb::CreateCompilationError(fb, request_id);
    return wrap_fb(fb, offset);
}

flatbuffers::DetachedBuffer create_ack_response(uint32_t request_id) {
    flatbuffers::FlatBufferBuilder fb;
    auto offset = mobsya::fb::CreateRequestCompleted(fb, request_id);
    return wrap_fb(fb, offset);
}

flatbuffers::DetachedBuffer serialize_aseba_vm_description(uint32_t request_id, const mobsya::aseba_node& n,
                                                           const aseba_node_registery::node_id& id) {

    Aseba::TargetDescription desc = n.vm_description();
    flatbuffers::FlatBufferBuilder fb;
    std::vector<flatbuffers::Offset<fb::NamedVariable>> variables_vector;
    std::vector<flatbuffers::Offset<fb::LocalEvent>> events_vector;
    std::vector<flatbuffers::Offset<fb::NativeFunction>> functions_vector;

    int i = 0;
    for(auto&& v : desc.namedVariables) {
        variables_vector.emplace_back(
            fb::CreateNamedVariable(fb, i++, fb.CreateString(Aseba::WStringToUTF8(v.name)), v.size));
    }

    i = 0;
    for(auto&& v : desc.localEvents) {
        events_vector.emplace_back(fb::CreateLocalEvent(fb, i++, fb.CreateString(Aseba::WStringToUTF8(v.name)),
                                                        fb.CreateString(Aseba::WStringToUTF8(v.description))));
    }

    i = 0;
    for(auto&& v : desc.nativeFunctions) {
        std::vector<flatbuffers::Offset<fb::NativeFunctionParameter>> params;
        for(auto&& p : v.parameters) {
            params.emplace_back(
                fb::CreateNativeFunctionParameter(fb, fb.CreateString(Aseba::WStringToUTF8(p.name)), p.size));
        }
        functions_vector.emplace_back(fb::CreateNativeFunction(fb, i++, fb.CreateString(Aseba::WStringToUTF8(v.name)),
                                                               fb.CreateString(Aseba::WStringToUTF8(v.description)),
                                                               fb.CreateVector(params)));
    }

    auto offset = CreateNodeAsebaVMDescription(fb, request_id, id.fb(fb), desc.bytecodeSize, desc.variablesSize,
                                               desc.stackSize, fb.CreateVector(variables_vector),
                                               fb.CreateVector(events_vector), fb.CreateVector(functions_vector));
    return wrap_fb(fb, offset);
}
}  // namespace mobsya
