#include "error.h"

namespace mobsya {
	struct usb_device_identifier {
    uint16_t vendor_id;
    uint16_t product_id;
};

inline bool operator==(const usb_device_identifier& a, const usb_device_identifier& b) {
    return a.vendor_id == b.vendor_id && a.product_id == b.product_id;
}
}