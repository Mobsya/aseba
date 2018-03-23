#include "ThymioInfo.h"

namespace mobsya {

ThymioInfo::ThymioInfo(std::shared_ptr<AbstractThymioInfoPrivate>&& data)
    : m_data(std::move(data)) {
}

QString ThymioInfo::name() const {
    return m_data->name();
}

ThymioInfo::DeviceProvider ThymioInfo::provider() const {
    return m_data->type();
}

const AbstractThymioInfoPrivate* ThymioInfo::data() const {
    return m_data.get();
}

bool ThymioInfo::operator==(const ThymioInfo& other) const {
    if(this->provider() != other.provider())
        return false;
    return m_data->equals(other);
}

AbstractThymioInfoPrivate::AbstractThymioInfoPrivate(ThymioInfo::DeviceProvider type)
    : m_type(type) {
}

AbstractThymioInfoPrivate::~AbstractThymioInfoPrivate() {
}

ThymioInfo::DeviceProvider AbstractThymioInfoPrivate::type() const {
    return m_type;
}

}    // namespace mobsya
