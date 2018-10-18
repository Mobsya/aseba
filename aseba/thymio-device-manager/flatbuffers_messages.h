#pragma once
#include <aseba/common/utils/utils.h>
#include <aseba/flatbuffers/thymio_generated.h>
#include <flatbuffers/flatbuffers.h>
#include <flatbuffers/flexbuffers.h>
#include "aseba_node.h"
#include "aseba_node_registery.h"
#include "property_flexbuffer.h"
#include <aseba/flatbuffers/fb_message_ptr.h>

namespace mobsya {

using vm_language = fb::ProgrammingLanguage;

tagged_detached_flatbuffer create_nodes_list_request() {
    flatbuffers::FlatBufferBuilder fb;
    auto offset = mobsya::fb::CreateRequestListOfNodes(fb);
    return wrap_fb(fb, offset);
}

tagged_detached_flatbuffer create_error_response(uint32_t request_id, fb::ErrorType error) {
    flatbuffers::FlatBufferBuilder fb;
    auto offset = mobsya::fb::CreateError(fb, request_id, error);
    return wrap_fb(fb, offset);
}

tagged_detached_flatbuffer create_ack_response(uint32_t request_id) {
    flatbuffers::FlatBufferBuilder fb;
    auto offset = mobsya::fb::CreateRequestCompleted(fb, request_id);
    return wrap_fb(fb, offset);
}

tagged_detached_flatbuffer create_set_breakpoint_response(uint32_t request_id, fb::ErrorType error,
                                                          const aseba_node::breakpoints& fbs) {
    flatbuffers::FlatBufferBuilder fb;
    std::vector<flatbuffers::Offset<fb::Breakpoint>> offsets;
    std::transform(fbs.begin(), fbs.end(), std::back_inserter(offsets),
                   [&fb](const breakpoint& bp) { return mobsya::fb::CreateBreakpoint(fb, bp.line); });
    auto vecOffset = fb.CreateVector(offsets);
    auto offset = mobsya::fb::CreateSetBreakpointsResponse(fb, request_id, error, vecOffset);
    return wrap_fb(fb, offset);
}

tagged_detached_flatbuffer create_compilation_result_response(uint32_t request_id,
                                                              const aseba_node::compilation_result& result) {
    flatbuffers::FlatBufferBuilder fb;
    if(result.error) {
        auto msgOffset = fb.CreateString(result.error->msg);
        auto offset = mobsya::fb::CreateCompilationResultFailure(fb, request_id, msgOffset, result.error->character,
                                                                 result.error->line, result.error->colum);
        return wrap_fb(fb, offset);
    }
    auto offset = mobsya::fb::CreateCompilationResultSuccess(
        fb, request_id, result.result->bytecode_size, result.result->bytecode_total_size, result.result->variables_size,
        result.result->variables_total_size);
    return wrap_fb(fb, offset);
}

tagged_detached_flatbuffer serialize_aseba_vm_description(uint32_t request_id, const mobsya::aseba_node& n,
                                                          const aseba_node_registery::node_id& id) {

    Aseba::TargetDescription desc = n.vm_description();
    flatbuffers::FlatBufferBuilder fb;
    std::vector<flatbuffers::Offset<fb::AsebaNamedVariable>> variables_vector;
    std::vector<flatbuffers::Offset<fb::AsebaEvent>> events_vector;
    std::vector<flatbuffers::Offset<fb::AsebaNativeFunction>> functions_vector;

    int i = 0;
    for(auto&& v : desc.namedVariables) {
        variables_vector.emplace_back(
            fb::CreateAsebaNamedVariable(fb, i++, fb.CreateString(Aseba::WStringToUTF8(v.name)), v.size));
    }

    i = 0;
    for(auto&& v : desc.localEvents) {
        events_vector.emplace_back(fb::CreateAsebaEvent(fb, i++, fb.CreateString(Aseba::WStringToUTF8(v.name)),
                                                        fb.CreateString(Aseba::WStringToUTF8(v.description))));
    }

    i = 0;
    for(auto&& v : desc.nativeFunctions) {
        std::vector<flatbuffers::Offset<fb::AsebaNativeFunctionParameter>> params;
        for(auto&& p : v.parameters) {
            params.emplace_back(
                fb::CreateAsebaNativeFunctionParameter(fb, fb.CreateString(Aseba::WStringToUTF8(p.name)), p.size));
        }
        functions_vector.emplace_back(fb::CreateAsebaNativeFunction(
            fb, i++, fb.CreateString(Aseba::WStringToUTF8(v.name)),
            fb.CreateString(Aseba::WStringToUTF8(v.description)), fb.CreateVector(params)));
    }

    auto offset = CreateNodeAsebaVMDescription(fb, request_id, id.fb(fb), desc.bytecodeSize, desc.variablesSize,
                                               desc.stackSize, fb.CreateVector(variables_vector),
                                               fb.CreateVector(events_vector), fb.CreateVector(functions_vector));
    return wrap_fb(fb, offset);
}

namespace detail {
    auto serialize_variables(flatbuffers::FlatBufferBuilder& fb, const mobsya::aseba_node::variables_map& vars) {
        flexbuffers::Builder flexbuilder;
        std::vector<flatbuffers::Offset<fb::NodeVariable>> varsOffsets;
        varsOffsets.reserve(vars.size());
        for(auto&& var : vars) {
            property_to_flexbuffer(var.second, flexbuilder);
            auto& vec = flexbuilder.GetBuffer();
            auto vecOffset = fb.CreateVector(vec);
            auto keyOffset = fb.CreateString(var.first);
            varsOffsets.push_back(fb::CreateNodeVariable(fb, keyOffset, vecOffset, var.second.is_constant));
            flexbuilder.Clear();
        }
        return fb.CreateVectorOfSortedTables(&varsOffsets);
    }
}  // namespace detail

tagged_detached_flatbuffer serialize_changed_variables(const mobsya::aseba_node& n,
                                                       const mobsya::aseba_node::variables_map& vars) {
    flatbuffers::FlatBufferBuilder fb;
    auto idOffset = n.uuid().fb(fb);
    auto varsOffset = detail::serialize_variables(fb, vars);
    auto offset = fb::CreateNodeVariablesChanged(fb, idOffset, varsOffset);
    return wrap_fb(fb, offset);
}

tagged_detached_flatbuffer serialize_events(const mobsya::aseba_node& n,
                                            const mobsya::aseba_node::variables_map& vars) {
    flatbuffers::FlatBufferBuilder fb;
    auto idOffset = n.uuid().fb(fb);
    auto varsOffset = detail::serialize_variables(fb, vars);
    auto offset = fb::CreateEventsEmitted(fb, idOffset, varsOffset);
    return wrap_fb(fb, offset);
}

tagged_detached_flatbuffer serialize_events_descriptions(const mobsya::aseba_node& n,
                                                         const mobsya::aseba_node::events_description_type& descs) {
    flatbuffers::FlatBufferBuilder fb;
    auto idOffset = n.uuid().fb(fb);
    std::vector<flatbuffers::Offset<fb::EventDescription>> descOffsets;
    descOffsets.reserve(descs.size());
    int i = 0;
    for(auto&& desc : descs) {
        auto str_offset = fb.CreateString(desc.name);
        auto descTable = fb::CreateEventDescription(fb, str_offset, desc.size, i++);
        descOffsets.push_back(descTable);
    }
    auto offset = fb::CreateEventsDescriptionChanged(fb, idOffset, fb.CreateVector(descOffsets));
    return wrap_fb(fb, offset);
}

tagged_detached_flatbuffer serialize_execution_state(const mobsya::aseba_node& n,
                                                     const mobsya::aseba_node::vm_execution_state& state) {
    flatbuffers::FlatBufferBuilder fb;
    auto idOffset = n.uuid().fb(fb);
    auto error_msg_offset = state.error_message ? fb.CreateString(*state.error_message) : 0;
    auto offset =
        fb::CreateVMExecutionStateChanged(fb, idOffset, state.state, state.line, state.error, error_msg_offset);
    return wrap_fb(fb, offset);
}

namespace detail {
    mobsya::aseba_node::variables_map
    variables(const flatbuffers::Vector<flatbuffers::Offset<fb::NodeVariable>>& buff) {
        mobsya::aseba_node::variables_map vars;
        vars.reserve(vars.size());
        for(const auto& offset : buff) {
            if(!offset->name() || !offset->value())
                continue;
            auto k = offset->name()->string_view();
            auto v = offset->value_flexbuffer_root();
            auto p = flexbuffer_to_property(v);
            auto constant = offset->constant();
            if(!p)
                continue;
            vars.insert_or_assign(std::string(k), aseba_node::variable(std::move(*p), constant));
        }
        return vars;
    }
}  // namespace detail

mobsya::aseba_node::variables_map variables(const fb::SetNodeVariables& msg) {
    if(!msg.vars())
        return {};
    return detail::variables(*msg.vars());
}

mobsya::aseba_node::variables_map events(const fb::SendEvents& msg) {
    if(!msg.events())
        return {};
    return detail::variables(*msg.events());
}


std::vector<mobsya::breakpoint> breakpoints(const fb::SetBreakpoints& msg) {
    if(!msg.breakpoints())
        return {};
    std::vector<mobsya::breakpoint> bps;
    for(auto&& offset : *msg.breakpoints()) {
        if(!offset)
            continue;
        bps.emplace_back(offset->line());
    }
    return bps;
}


}  // namespace mobsya
