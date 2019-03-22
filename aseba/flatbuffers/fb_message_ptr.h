#pragma once
#include <aseba/flatbuffers/thymio_generated.h>

namespace mobsya {

class fb_message_ptr {
public:
    fb_message_ptr() : m_msg(nullptr) {}
    fb_message_ptr(const fb_message_ptr&) = delete;
    fb_message_ptr(fb_message_ptr&& other) : m_data(std::move(other.m_data)), m_msg(other.m_msg) {
        other.m_msg = nullptr;
    }
    fb_message_ptr(std::vector<uint8_t>&& data) : m_data(std::move(data)) {
        m_msg = fb::GetMessage(m_data.data());
    }

    operator bool() const {
        return m_msg;
    }

    const fb::Message* operator->() const {
        return m_msg;
    }

    const fb::Message& operator*() const {
        return *m_msg;
    }

    fb::AnyMessage message_type() const {
        return m_msg->message_type();
    }

    template <typename T>
    const T* as() const {
        return m_msg->message_as<T>();
    }

private:
    std::vector<uint8_t> m_data;
    const fb::Message* m_msg;
};

struct tagged_detached_flatbuffer {
    flatbuffers::DetachedBuffer buffer;
    mobsya::fb::AnyMessage tag;
};

template <typename MessageType>
tagged_detached_flatbuffer wrap_fb(flatbuffers::FlatBufferBuilder& fb, const flatbuffers::Offset<MessageType>& offset) {
    constexpr auto type = mobsya::fb::AnyMessageTraits<MessageType>::enum_value;
    auto rootOffset = mobsya::fb::CreateMessage(fb, type, offset.Union());
    fb.Finish(rootOffset);
    auto x = fb.Release();
    return tagged_detached_flatbuffer{std::move(x), type};
}

}  // namespace mobsya
