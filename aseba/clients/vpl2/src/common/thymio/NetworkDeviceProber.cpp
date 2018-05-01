#include "NetworkDeviceProber.h"
#include <QTcpSocket>


namespace mobsya {


class NetworkThymioProviderInfo : public AbstractThymioProviderInfoPrivate {
public:
    NetworkThymioProviderInfo(QZeroConfService service)
        : AbstractThymioProviderInfoPrivate(ThymioProviderInfo::ProviderType::Tcp), m_service(service) {}

    QString name() const override {
        return m_service.name();
    }
    bool equals(const ThymioProviderInfo& other) override {
        if(other.type() != ThymioProviderInfo::ProviderType::Tcp)
            return false;
        auto port = static_cast<const NetworkThymioProviderInfo*>(other.data())->m_service;
        return port == m_service;
    }
    bool lt(const ThymioProviderInfo& other) override {
        if(other.type() != ThymioProviderInfo::ProviderType::Tcp)
            return false;
        auto service = static_cast<const NetworkThymioProviderInfo*>(other.data())->m_service;
        if(m_service.ip().toIPv4Address() == service.ip().toIPv4Address())
            return m_service.port() < service.port();
        return m_service.ip().toIPv4Address() < service.ip().toIPv4Address();
    }

    QZeroConfService m_service;
};

QZeroConfService dns_service_for_provider(const ThymioProviderInfo& info) {
    if(info.type() != ThymioProviderInfo::ProviderType::Tcp)
        return {};
    return static_cast<const NetworkThymioProviderInfo*>(info.data())->m_service;
}

NetworkDeviceProber::NetworkDeviceProber(QObject* parent)
    : AbstractDeviceProber(parent), m_register(new QZeroConf(this)) {

    connect(m_register, &QZeroConf::serviceAdded, this, &NetworkDeviceProber::onServiceAdded);
    connect(m_register, &QZeroConf::serviceUpdated, this, &NetworkDeviceProber::onServiceUpdated);
    connect(m_register, &QZeroConf::serviceRemoved, this, &NetworkDeviceProber::onServiceRemoved);
    m_register->startBrowser("_aseba._tcp");
}

NetworkDeviceProber::~NetworkDeviceProber() {}

std::vector<ThymioProviderInfo> NetworkDeviceProber::getDevices() {
    std::vector<ThymioProviderInfo> info;
    for(auto&& service : m_services) {
        info.emplace_back(std::make_shared<NetworkThymioProviderInfo>(service));
    }
    return info;
}
std::shared_ptr<DeviceQtConnection> NetworkDeviceProber::openConnection(const ThymioProviderInfo& thymio) {
    if(thymio.type() != ThymioProviderInfo::ProviderType::Tcp)
        return {};
    auto& service = static_cast<const NetworkThymioProviderInfo*>(thymio.data())->m_service;

    QTcpSocket* s = new QTcpSocket;
    s->connectToHost(service.ip(), service.port());
    auto con = std::make_shared<DeviceQtConnection>(thymio, s);
    connect(s, &QTcpSocket::connected, con.get(), &DeviceQtConnection::connectionStatusChanged);
    connect(s, &QTcpSocket::disconnected, con.get(), &DeviceQtConnection::connectionStatusChanged,
            Qt::QueuedConnection);
    return con;
}


void NetworkDeviceProber::onServiceAdded(QZeroConfService service) {
    if(service.ip().isLoopback() || service.ip().isEqual(QHostAddress::LocalHost))
        return;
    m_services.append(service);
    Q_EMIT availabilityChanged();
}

void NetworkDeviceProber::onServiceUpdated(QZeroConfService) {
    Q_EMIT availabilityChanged();
}

void NetworkDeviceProber::onServiceRemoved(QZeroConfService service) {
    m_services.removeAll(service);
    Q_EMIT availabilityChanged();
}
}  // namespace mobsya
