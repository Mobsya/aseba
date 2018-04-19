#include "ThymioProviderInfo.h"

namespace mobsya {

ThymioProviderInfo::ThymioProviderInfo(std::shared_ptr<AbstractThymioProviderInfoPrivate>&& data)
    : m_data(std::move(data)) {
}

QString ThymioProviderInfo::name() const {
    return m_data->name();
}

ThymioProviderInfo::ProviderType ThymioProviderInfo::type() const {
    return m_data->type();
}

const AbstractThymioProviderInfoPrivate* ThymioProviderInfo::data() const {
    return m_data.get();
}

bool ThymioProviderInfo::operator==(const ThymioProviderInfo& other) const {
    if(this->type() != other.type())
        return false;
    return m_data->equals(other);
}

bool ThymioProviderInfo::operator<(const ThymioProviderInfo& other) const {
    if(this->type() != other.type()) {
        return this->type() < other.type();
    }
    return m_data->lt(other);
}

AbstractThymioProviderInfoPrivate::AbstractThymioProviderInfoPrivate(
    ThymioProviderInfo::ProviderType type)
    : m_type(type) {
}

AbstractThymioProviderInfoPrivate::~AbstractThymioProviderInfoPrivate() {
}

ThymioProviderInfo::ProviderType AbstractThymioProviderInfoPrivate::type() const {
    return m_type;
}

}    // namespace mobsya
