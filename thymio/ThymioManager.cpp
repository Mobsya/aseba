#include "ThymioManager.h"
#include <QDebug>
#include <vector>

#ifdef Q_OS_ANDROID
#    include "AndroidSerialDeviceProber.h"
#endif
#if(defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)) || defined(Q_OS_MAC) || defined(Q_OS_WIN)
#    include "UsbSerialDeviceProber.h"
#endif

#include "NetworkDeviceProber.h"


namespace mobsya {

AbstractDeviceProber::AbstractDeviceProber(QObject* parent)
    : QObject(parent) {
}


ThymioManager::ThymioManager(QObject* parent)
    : QObject(parent) {

    // Thymio using the system-level serial driver only works on desktop OSes
    /*#if(defined(Q_OS_LINUX) && !defined(Q_OS_ANDROID)) || defined(Q_OS_MAC) || defined(Q_OS_WIN)
        auto desktopProbe = new UsbSerialDeviceProber(this);
        connect(desktopProbe, &UsbSerialDeviceProber::availabilityChanged, this,
                &ThymioManager::scanDevices);
        m_probes.push_back(desktopProbe);
    #endif
    #ifdef Q_OS_ANDROID
        auto androidProbe = AndroidSerialDeviceProber::instance();
        connect(androidProbe, &AndroidSerialDeviceProber::availabilityChanged, this,
                &ThymioManager::scanDevices);
        m_probes.push_back(androidProbe);
    #endif
    */
    auto networkProbe = new NetworkDeviceProber(this);
    connect(networkProbe, &NetworkDeviceProber::availabilityChanged, this,
            &ThymioManager::scanDevices);
    m_probes.push_back(networkProbe);

    scanDevices();
}


void ThymioManager::scanDevices() {
    std::vector<ThymioProviderInfo> new_thymios;
    for(auto&& probe : m_probes) {
        auto thymios = probe->getThymios();
        std::move(std::begin(thymios), std::end(thymios), std::back_inserter(new_thymios));
    }
    m_thymios.erase(std::remove_if(std::begin(m_thymios), std::end(m_thymios),
                                   [&new_thymios](const ThymioProviderInfo& thymio) {
                                       return std::find(std::begin(new_thymios),
                                                        std::end(new_thymios),
                                                        thymio) == std::end(new_thymios);
                                   }),
                    std::end(m_thymios));
    std::copy_if(std::make_move_iterator(std::begin(new_thymios)),
                 std::make_move_iterator(std::end(new_thymios)), std::back_inserter(m_thymios),
                 [this](const ThymioProviderInfo& thymio) {
                     return std::find(std::begin(m_thymios), std::end(m_thymios), thymio) ==
                            std::end(m_thymios);
                 });

    qDebug() << ">>>-----";
    for(auto&& thymio : m_thymios) {
        qDebug() << thymio.name() << (int)thymio.type();
    }
    qDebug() << "<<<-----";
}

std::unique_ptr<DeviceQtConnection>
ThymioManager::openConnection(const ThymioProviderInfo& thymio) {
    for(auto&& probe : m_probes) {
        auto iod = probe->openConnection(thymio);
        if(iod)
            return std::make_unique<DeviceQtConnection>(iod.release());
    }
    return {};
}

ThymioProviderInfo ThymioManager::first() const {
    return m_thymios.front();
}

bool ThymioManager::hasDevice() const {
    return m_thymios.size() > 0;
}

}    // namespace mobsya
