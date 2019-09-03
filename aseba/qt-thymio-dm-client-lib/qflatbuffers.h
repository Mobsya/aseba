#pragma once
#include <aseba/flatbuffers/fb_message_ptr.h>
#include <QString>
#include <QUuid>
#include <QtEndian>
#include <QVariant>
#include <QtDebug>

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
            return QVariant::fromValue<qint64>(r.AsInt64());
        }
        if(r.IsFloat()) {
            return QVariant::fromValue<double>(r.AsDouble());
        }
        if(r.IsBool()) {
            return QVariant::fromValue<bool>(r.AsBool());
        }
        if(r.IsNull()) {
            return QVariant();
        }
        if(r.IsString()) {
            const auto& data = r.AsBlob();
            auto str = QString::fromUtf8(reinterpret_cast<const char*>(data.data()), int(data.size()));
            return QVariant(str);
        }
        if(r.IsVector()) {
            auto v = r.AsVector();
            QVariantList l;
            l.reserve(int(v.size()));
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

    namespace detail {
        inline void to_flexbuffer(const QVariant& p, flexbuffers::Builder& b) {
            switch(p.type()) {
                case QVariant::Invalid: {
                    b.Null();
                    break;
                }
                case QVariant::Bool: {
                    b.Bool(p.toBool());
                    break;
                }
                case QVariant::Int:
                case QVariant::LongLong: {
                    b.Int(p.toInt());
                    break;
                }
                case QVariant::UInt:
                case QVariant::ULongLong: {
                    b.UInt(p.toULongLong());
                    break;
                }
                case QVariant::Double: {
                    b.Double(p.toDouble());
                    break;
                }
                case QVariant::String: {
                    QByteArray utf8 = p.toString().toUtf8();
                    b.String(utf8.data(), utf8.size());
                    break;
                }
                case QVariant::List:
                case QVariant::StringList: {
                    auto start = b.StartVector();
                    for(auto&& v : p.toList()) {
                        to_flexbuffer(v, b);
                    }
                    b.EndVector(start, false, false);
                    break;
                }
                case QVariant::Map: {
                    auto start = b.StartMap();
                    for(auto&& v : p.toMap().toStdMap()) {
                        QByteArray utf8 = v.first.toUtf8();
                        b.Key(utf8.data(), utf8.size());
                        to_flexbuffer(v.second, b);
                    }
                    b.EndMap(start);
                    break;
                }
                default: qWarning() << QStringLiteral("Cannot serialize %1 to flexbuffer").arg(p.typeName());
            }
        }
    }  // namespace detail
    inline void to_flexbuffer(const QVariant& p, flexbuffers::Builder& b) {
        detail::to_flexbuffer(p, b);
        b.Finish();
    }
}  // namespace qfb
}  // namespace mobsya