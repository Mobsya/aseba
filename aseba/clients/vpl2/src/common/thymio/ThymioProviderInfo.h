#pragma once
#include <QObject>
#include <QIODevice>
#include <memory>

namespace mobsya {

class AbstractThymioProviderInfoPrivate;
class ThymioProviderInfo {
    Q_GADGET

public:
    enum class ProviderType { Serial, Tcp, AndroidSerial };
    ThymioProviderInfo(std::shared_ptr<AbstractThymioProviderInfoPrivate>&& data);
    QString name() const;
    ProviderType type() const;
    bool operator==(const ThymioProviderInfo& other) const;
    bool operator<(const ThymioProviderInfo& other) const;


    const AbstractThymioProviderInfoPrivate* data() const;

private:
    std::shared_ptr<AbstractThymioProviderInfoPrivate> m_data;
};


class AbstractThymioProviderInfoPrivate {
public:
    AbstractThymioProviderInfoPrivate(ThymioProviderInfo::ProviderType type);
    virtual ~AbstractThymioProviderInfoPrivate();

    virtual QString name() const = 0;
    ThymioProviderInfo::ProviderType type() const;
    virtual bool equals(const ThymioProviderInfo& other) = 0;
    virtual bool lt(const ThymioProviderInfo& other) = 0;

private:
    ThymioProviderInfo::ProviderType m_type;
};

}  // namespace mobsya
