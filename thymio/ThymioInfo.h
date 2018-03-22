#pragma once
#include <QObject>
#include <QIODevice>
#include <memory>

namespace mobsya {

class AbstractThymioInfoPrivate;
class ThymioInfo {
    Q_GADGET

public:
    enum class DeviceProvider { Serial, Tcp, AndroidSerial };
    ThymioInfo(std::unique_ptr<AbstractThymioInfoPrivate>&& data);
    QString name() const;
    DeviceProvider provider() const;
    bool operator==(const ThymioInfo& other) const;


    const AbstractThymioInfoPrivate* data() const;

private:
    std::unique_ptr<AbstractThymioInfoPrivate> m_data;
};


class AbstractThymioInfoPrivate {
public:
    AbstractThymioInfoPrivate(ThymioInfo::DeviceProvider type);
    virtual ~AbstractThymioInfoPrivate();

    virtual QString name() const = 0;
    ThymioInfo::DeviceProvider type() const;
    virtual bool equals(const ThymioInfo& other) = 0;

private:
    ThymioInfo::DeviceProvider m_type;
};

}    // namespace mobsya
