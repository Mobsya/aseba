#pragma once
#include "ThymioManager.h"
#include <QThread>
#include <libusb.h>

namespace mobsya {


class UsbSerialDeviceProberThread : public QThread {
    Q_OBJECT
public:
    void run() override;
Q_SIGNALS:
    void availabilityChanged();

private:
    int hotplug_callback(struct libusb_context* ctx, struct libusb_device* dev,
                         libusb_hotplug_event event);
    friend int hotplug_callback(struct libusb_context* ctx, struct libusb_device* dev,
                                libusb_hotplug_event event, void* user_data);
};

class UsbSerialDeviceProber : public AbstractDeviceProber {
    Q_OBJECT
public:
    UsbSerialDeviceProber(QObject* parent = nullptr);
    ~UsbSerialDeviceProber();
    std::vector<ThymioProviderInfo> getDevices() override;
    std::unique_ptr<QIODevice> openConnection(const ThymioProviderInfo& thymio) override;

private:
    void scan();
    UsbSerialDeviceProberThread m_thread;
};

}    // namespace mobsya
