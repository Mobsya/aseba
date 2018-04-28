#pragma once
#include <libusb/libusb.h>

namespace mobsya {

class usb_device {
public:
    ~usb_device();
    void set_native_device(libusb_device*);

private:
    libusb_device* m_impl = nullptr;
};

}  // namespace mobsya
