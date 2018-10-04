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

template <typename MessageType>
flatbuffers::DetachedBuffer wrap_fb(flatbuffers::FlatBufferBuilder& fb,
                                    const flatbuffers::Offset<MessageType>& offset) {
    auto rootOffset =
        mobsya::fb::CreateMessage(fb, mobsya::fb::AnyMessageTraits<MessageType>::enum_value, offset.Union());
    fb.Finish(rootOffset);
    auto x = fb.ReleaseBufferPointer();
    return x;
}

}  // namespace mobsya
