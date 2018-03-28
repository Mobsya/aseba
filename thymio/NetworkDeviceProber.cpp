#include "NetworkDeviceProber.h"


namespace mobsya {

NetworkDeviceProber::NetworkDeviceProber(QObject* parent)
    : AbstractDeviceProber(parent)
    , m_register(new QZeroConf(this)) {

    connect(m_register, &QZeroConf::serviceAdded, this, &NetworkDeviceProber::onServiceAdded);
    connect(m_register, &QZeroConf::serviceUpdated, this, &NetworkDeviceProber::onServiceUpdated);
    connect(m_register, &QZeroConf::serviceRemoved, this, &NetworkDeviceProber::onServiceRemoved);
    m_register->startBrowser("_aseba._tcp");
}

NetworkDeviceProber::~NetworkDeviceProber() {
}

std::vector<ThymioInfo> NetworkDeviceProber::getThymios() {
    return {};
}
std::unique_ptr<QIODevice> NetworkDeviceProber::openConnection(const ThymioInfo& thymio) {
    return {};
}


void NetworkDeviceProber::onServiceAdded(QZeroConfService service) {
    updateService(service);
}

void NetworkDeviceProber::onServiceUpdated(QZeroConfService service) {
    updateService(service);
}

void NetworkDeviceProber::onServiceRemoved(QZeroConfService service) {
    updateService(service);
}

void NetworkDeviceProber::updateService(QZeroConfService service) {
    qDebug() << service.domain() << service.host() << service.port() << service.name();
    qDebug() << service.txt();
}

}    // namespace mobsya
