#include "usbdevice.h"

namespace mobsya {

usb_device::~usb_device() {
    libusb_unref_device(m_impl);
}

void usb_device::set_native_device(libusb_device* d) {
    m_impl = d;
    libusb_ref_device(m_impl);
}

}  // namespace mobsya
