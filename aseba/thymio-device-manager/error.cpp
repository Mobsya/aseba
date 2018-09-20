#include "error.h"

namespace mobsya {
#ifdef MOBSYA_TDM_ENABLE_USB
namespace usb {

    const boost::system::error_category& get_usb_error_category() {
        static const usb_error_category usb_error_cat;
        return usb_error_cat;
    }


    const char* usb_error_category::name() const noexcept {
        return "libusb";
    }

    std::string usb_error_category::message(int ev) const {
        switch(error_code(ev)) {
            case error_code::io_error: return "i/o error";
            case error_code::invalid_argument: return "invalid argument";
            case error_code::access_denied: return "access denied";
            case error_code::no_such_device: return "no such device";
            case error_code::not_found: return "not found";
            case error_code::busy: return "busy";
            case error_code::overflow: return "overflow";
            case error_code::broken_pipe: return "brocken pipe";
            case error_code::timed_out: return "timed out";
            case error_code::interrupted: return "interrupted";
            case error_code::no_memory: return "not enough memory";
            case error_code::not_supported: return "operation not supported";
        }
        return {};
    }

}  // namespace usb
#endif
}  // namespace mobsya
