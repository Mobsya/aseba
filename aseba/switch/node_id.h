#pragma once
#include <boost/container_hash/hash.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>  //for fmt
#include <aseba/flatbuffers/thymio_generated.h>

namespace mobsya {
class node_id : public boost::uuids::uuid {
public:
    node_id(const boost::uuids::uuid& id) : uuid(id) {}
    node_id(const mobsya::fb::NodeId* const id) {
        const flatbuffers::Vector<uint8_t>* d = id->id();
        if(d->size() == 16)
            std::copy_n(d->begin(), 16, begin());
    }

    auto fb(flatbuffers::FlatBufferBuilder& fb) const {
        return mobsya::fb::CreateNodeId(fb, fb.CreateVector(begin(), size()));
    }

    /*Return a short node id compatible with thyimio 2, but with much less entropy*/
    uint16_t short_for_tymio2() const {
        return *reinterpret_cast<const uint16_t*>(data);
    }
};
}  // namespace mobsya

namespace std {
template <>
struct hash<mobsya::node_id> {
    size_t operator()(const mobsya::node_id& uid) const {
        return boost::hash<boost::uuids::uuid>()(uid);
    }
};

}  // namespace std
