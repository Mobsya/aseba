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


    inline QString as_qstring(const flatbuffers::String& str) {
        return QString::fromUtf8(str.data(), str.size());
    }

    inline QString as_qstring(const flatbuffers::String* const str) {
        if(!str)
            return {};
        return as_qstring(*str);
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

    inline QUuid uuid(const fb::NodeIdT* const table) {
        return table ? uuid(table->id) : QUuid();
    }

    inline QVariant to_qvariant(const flexbuffers::Reference& r) {
        if(r.IsIntOrUint()) {
            return QVariant::fromValue<quint64>(r.AsInt64());
        }
        if(r.IsFloat()) {
            return QVariant::fromValue<quint64>(r.AsDouble());
        }
        if(r.IsBool()) {
            return QVariant::fromValue<quint64>(r.AsBool());
        }
        if(r.IsNull()) {
            return QVariant();
        }
        if(r.IsString()) {
            const auto& data = r.AsBlob();
            auto str = QString::fromUtf8(reinterpret_cast<const char*>(data.data()), data.size());
            return QVariant(str);
        }
        if(r.IsVector()) {
            auto v = r.AsVector();
            QVariantList l;
            l.reserve(v.size());
            for(size_t i = 0; i < v.size(); i++) {
                auto p = to_qvariant(v[i]);
                l.push_back(p);
            }
            return l;
        }
        if(r.IsMap()) {
            auto m = r.AsMap();
            auto keys = m.Keys();
            QVariantMap o;
            for(size_t i = 0; i < keys.size(); i++) {
                auto k = keys[i].AsKey();
                if(!k)
                    continue;
                auto p = to_qvariant(m[k]);
                o.insert(k, p);
            }
            return o;
        }
        return {};
    }
}  // namespace qfb
}  // namespace mobsya