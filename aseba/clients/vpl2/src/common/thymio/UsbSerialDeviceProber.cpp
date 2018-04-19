#include "UsbSerialDeviceProber.h"
#include <QThread>
#include <QSerialPortInfo>
#include <QtSerialPort>
#include <QtDebug>

namespace mobsya {

class LibUSB {
public:
    LibUSB() {
        libusb_init(NULL);
    }
    ~LibUSB() {
        libusb_exit(NULL);
    }
};
static LibUSB lib_usb_handle;
constexpr int EPFL_VENDOR_ID = 0x0617;


class UsbSerialThymioProviderInfo : public AbstractThymioProviderInfoPrivate {
public:
    UsbSerialThymioProviderInfo(QString portName, QString deviceName)
        : AbstractThymioProviderInfoPrivate(ThymioProviderInfo::ProviderType::Serial)
        , m_portName(portName)
        , m_deviceName(deviceName) {
    }

    QString name() const override {
        return m_deviceName;
    }
    bool equals(const ThymioProviderInfo& other) override {
        if(other.type() != ThymioProviderInfo::ProviderType::Serial)
            return false;
        auto port = static_cast<const UsbSerialThymioProviderInfo*>(other.data())->m_portName;
        return port == m_portName;
    }
    bool lt(const ThymioProviderInfo& other) override {
        if(other.type() != ThymioProviderInfo::ProviderType::Serial)
            return false;
        auto port = static_cast<const UsbSerialThymioProviderInfo*>(other.data())->m_portName;
        return port < m_portName;
    }

    QString m_portName;
    QString m_deviceName;
};


int hotplug_callback(struct libusb_context* ctx, struct libusb_device* dev,
                     libusb_hotplug_event event, void* user_data) {
    auto object = reinterpret_cast<UsbSerialDeviceProberThread*>(user_data);
    return object->hotplug_callback(ctx, dev, event);
}
void UsbSerialDeviceProberThread::run() {
    libusb_hotplug_callback_handle handle;

    int rc = libusb_hotplug_register_callback(
        nullptr,
        libusb_hotplug_event(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED |
                             LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT),
        libusb_hotplug_flag(0), EPFL_VENDOR_ID, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY,
        mobsya::hotplug_callback, this, &handle);

    if(LIBUSB_SUCCESS != rc) {
        return;
    }

    while(!this->isInterruptionRequested()) {
        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        libusb_handle_events_timeout_completed(nullptr, &tv, nullptr);
    }

    libusb_hotplug_deregister_callback(nullptr, handle);
}

int UsbSerialDeviceProberThread::hotplug_callback(struct libusb_context*, struct libusb_device*,
                                                  libusb_hotplug_event) {
    Q_EMIT availabilityChanged();
    return 0;
}

UsbSerialDeviceProber::UsbSerialDeviceProber(QObject* parent)
    : AbstractDeviceProber(parent) {

    connect(&m_thread, &UsbSerialDeviceProberThread::availabilityChanged, this,
            &UsbSerialDeviceProber::availabilityChanged);
    m_thread.start();
}

std::vector<ThymioProviderInfo> UsbSerialDeviceProber::getDevices() {
    std::vector<ThymioProviderInfo> compatible_ports;
    const auto serial_ports = QSerialPortInfo::availablePorts();
    for(auto&& port : qAsConst(serial_ports)) {
        if(!port.hasProductIdentifier())
            continue;
        if(port.vendorIdentifier() == EPFL_VENDOR_ID) {
            compatible_ports.emplace_back(
                std::make_unique<UsbSerialThymioProviderInfo>(port.portName(), port.description()));
        }
    }
    return compatible_ports;
}

std::shared_ptr<DeviceQtConnection>
UsbSerialDeviceProber::openConnection(const ThymioProviderInfo& thymio) {
    if(thymio.type() != ThymioProviderInfo::ProviderType::Serial)
        return nullptr;
    auto port = static_cast<const UsbSerialThymioProviderInfo*>(thymio.data())->m_portName;
    auto connection = std::make_unique<QSerialPort>(port);
    if(!connection->open(QIODevice::ReadWrite)) {
        return {};
    }
    connection->setBaudRate(QSerialPort::Baud115200);
    connection->setParity(QSerialPort::NoParity);
    connection->setStopBits(QSerialPort::OneStop);
    connection->setFlowControl(QSerialPort::NoFlowControl);
    connection->setDataTerminalReady(true);

    auto wrapper = std::make_shared<DeviceQtConnection>(thymio, connection.get());
    connect(connection.get(), &QSerialPort::errorOccurred, wrapper.get(),
            &DeviceQtConnection::connectionStatusChanged);
    connection.release();
    return wrapper;
}    // namespace mobsya

UsbSerialDeviceProber::~UsbSerialDeviceProber() {
    m_thread.requestInterruption();
    m_thread.wait();
}

}    // namespace mobsya
