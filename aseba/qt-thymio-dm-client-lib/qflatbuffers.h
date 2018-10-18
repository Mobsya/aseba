#pragma once
#include <aseba/flatbuffers/fb_message_ptr.h>
#include <QString>
#include <QUuid>
#include <QtEndian>

namespace mobsya {
namespace qfb {
    inline auto add_string(flatbuffers::FlatBufferBuilder& fb, const QString& str) {
        QByteArray utf8 = str.toUtf8();
        return fb.CreateString(utf8.data(), utf8.size());
    }

    template <typename T>
    T to_uuid_part(const unsigned char* c) {
        return qToBigEndian(*(reinterpret_cast<const T*>(c)));
    }

    inline QUuid uuid(const std::vector<uint8_t>& bytes) {
        return QUuid(to_uuid_part<uint32_t>(bytes.data()), to_uuid_part<uint16_t>(bytes.data() + 4),
                     to_uuid_part<uint16_t>(bytes.data() + 6), to_uuid_part<uint8_t>(bytes.data() + 8),
                     to_uuid_part<uint8_t>(bytes.data() + 9), to_uuid_part<uint8_t>(bytes.data() + 10),
                     to_uuid_part<uint8_t>(bytes.data() + 11), to_uuid_part<uint8_t>(bytes.data() + 12),
                     to_uuid_part<uint8_t>(bytes.data() + 13), to_uuid_part<uint8_t>(bytes.data() + 14),
                     to_uuid_part<uint8_t>(bytes.data() + 15));
    }

    inline QUuid uuid(const fb::NodeIdT& table) {
        return uuid(table.id);
    }


}  // namespace qfb
}  // namespace mobsya