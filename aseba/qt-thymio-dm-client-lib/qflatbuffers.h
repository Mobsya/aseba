#pragma once
#include <aseba/flatbuffers/fb_message_ptr.h>
#include <QString>

namespace mobsya {
namespace qfb {
    inline auto add_string(flatbuffers::FlatBufferBuilder& fb, const QString& str) {
        QByteArray utf8 = str.toUtf8();
        return fb.CreateString(utf8.data(), utf8.size());
    }
}  // namespace qfb
}  // namespace mobsya