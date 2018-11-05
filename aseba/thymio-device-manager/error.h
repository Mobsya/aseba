#pragma once
#ifdef MOBSYA_TDM_ENABLE_USB
#include <libusb/libusb.h>
#endif
#include <boost/asio/error.hpp>
#include "utils.h"

namespace mobsya {

enum class error_code {
    invalid_object = 0x01,
    no_such_variable,
    incompatible_variable_type,
    invalid_aesl,
};

class tdm_error_category : public boost::system::error_category {
public:
    tdm_error_category() : boost::system::error_category() {}
    const char* name() const noexcept override;
    std::string message(int ev) const override;
};

const boost::system::error_category& get_tdm_error_category();

inline boost::system::error_code make_error_code(error_code e) {
    return boost::system::error_code(static_cast<int>(e), tdm_error_category());
}

inline boost::system::error_code make_error_code(int e) {
    return boost::system::error_code(e, tdm_error_category());
}

inline auto make_unexpected(error_code e) {
    return tl::make_unexpected(make_error_code(e));
}

inline auto make_unexpected(int e) {
    return make_unexpected(error_code(e));
}


#ifdef MOBSYA_TDM_ENABLE_USB
namespace usb {

    enum class error_code {
        io_error = LIBUSB_ERROR_IO,
        invalid_argument = LIBUSB_ERROR_INVALID_PARAM,
        access_denied = LIBUSB_ERROR_ACCESS,
        no_such_device = LIBUSB_ERROR_NO_DEVICE,
        not_found = LIBUSB_ERROR_NOT_FOUND,
        busy = LIBUSB_ERROR_BUSY,
        overflow = LIBUSB_ERROR_OVERFLOW,
        broken_pipe = LIBUSB_ERROR_PIPE,
        timed_out = LIBUSB_ERROR_TIMEOUT,
        interrupted = LIBUSB_ERROR_INTERRUPTED,
        no_memory = LIBUSB_ERROR_NO_MEM,
        not_supported = LIBUSB_ERROR_NOT_SUPPORTED,
    };

    class usb_error_category : public boost::system::error_category {
    public:
        usb_error_category() : boost::system::error_category() {}
        const char* name() const noexcept override;
        std::string message(int ev) const override;
    };

    const boost::system::error_category& get_usb_error_category();

    inline boost::system::error_code make_error_code(error_code e) {
        return boost::system::error_code(static_cast<int>(e), get_usb_error_category());
    }
    inline boost::system::error_code make_error_code(int e) {
        return boost::system::error_code(e, get_usb_error_category());
    }


    inline boost::system::error_code make_error_code_from_transfer(int e) {
        int r = LIBUSB_ERROR_OTHER;
        switch(e) {
            case LIBUSB_TRANSFER_ERROR: r = LIBUSB_ERROR_IO; break;
            case LIBUSB_TRANSFER_TIMED_OUT: r = LIBUSB_ERROR_TIMEOUT; break;
            case LIBUSB_TRANSFER_CANCELLED: r = LIBUSB_ERROR_INTERRUPTED; break;
            case LIBUSB_TRANSFER_NO_DEVICE: r = LIBUSB_ERROR_NO_DEVICE; break;
            case LIBUSB_TRANSFER_OVERFLOW: r = LIBUSB_ERROR_OVERFLOW; break;
            case LIBUSB_TRANSFER_STALL: r = LIBUSB_ERROR_IO; break;
        }
        return boost::system::error_code(r, get_usb_error_category());
    }


    inline auto make_unexpected(error_code e) {
        return tl::make_unexpected(make_error_code(e));
    }

    inline auto make_unexpected(libusb_error e) {
        return make_unexpected(error_code(e));
    }
    inline auto make_unexpected(int e) {
        return make_unexpected(error_code(e));
    }

}  // namespace usb
#endif
}  // namespace mobsya

#ifdef MOBSYA_TDM_ENABLE_USB
namespace boost {
namespace system {
    template <>
    struct is_error_code_enum<mobsya::usb::error_code> {
        static const bool value = true;
    };
    template <>
    struct is_error_code_enum<mobsya::tdm_error_category> {
        static const bool value = true;
    };
}  // namespace system
}  // namespace boost
#endif