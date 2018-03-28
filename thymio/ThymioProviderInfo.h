#pragma once
#include <QObject>
#include <QIODevice>
#include <memory>

namespace mobsya {

class AbstractThymioProviderInfoPrivate;
class ThymioProviderInfo {
    Q_GADGET

public:
    enum class DeviceProvider { Serial, Tcp, AndroidSerial };
    ThymioProviderInfo(std::shared_ptr<AbstractThymioProviderInfoPrivate>&& data);
    QString name() const;
    DeviceProvider type() const;
    bool operator==(const ThymioProviderInfo& other) const;


    const AbstractThymioProviderInfoPrivate* data() const;

private:
    std::shared_ptr<AbstractThymioProviderInfoPrivate> m_data;
};


class AbstractThymioProviderInfoPrivate {
public:
    AbstractThymioProviderInfoPrivate(ThymioProviderInfo::DeviceProvider type);
    virtual ~AbstractThymioProviderInfoPrivate();

    virtual QString name() const = 0;
    ThymioProviderInfo::DeviceProvider type() const;
    virtual bool equals(const ThymioProviderInfo& other) = 0;

private:
    ThymioProviderInfo::DeviceProvider m_type;
};

}    // namespace mobsya
