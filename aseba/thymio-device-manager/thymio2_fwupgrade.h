#pragma once
#include <map>
#include <range/v3/range/conversion.hpp>
#include <range/v3/view/span.hpp>
#include "utils.h"
#include "log.h"
#ifdef MOBSYA_TDM_ENABLE_USB
#    include "libusb/libusb.h"
#endif

namespace mobsya {

using thymio2_firmware_data = std::vector<std::pair<uint32_t, std::vector<uint8_t>>>;
using firmware_upgrade_callback = std::function<void(boost::system::error_code ec, double progress, bool complete)>;

namespace details {

    using chunks = std::map<uint32_t, std::vector<uint8_t>>;
    using page_map = chunks;

    enum class hex_file_parse_error {
        no_error = 0,
        file_error,
        invalid_record,
        wrong_checksum,
        unknown_record,
        file_truncated
    };

    tl::expected<chunks, hex_file_parse_error> read_hex_file(std::string hex_data);
    tl::expected<page_map, hex_file_parse_error> extract_pages_from_hex(std::string hex_data);

    void upgrade_thymio2_serial_endpoint(std::string path, const thymio2_firmware_data& data, uint16_t id,
                                         firmware_upgrade_callback cb);

#ifdef MOBSYA_TDM_ENABLE_USB
    void upgrade_thymio2_usb_endpoint(libusb_device_handle* d, const thymio2_firmware_data& data, uint16_t id,
                                      firmware_upgrade_callback cb);
#endif

}  // namespace details


#ifdef MOBSYA_TDM_ENABLE_SERIAL
void upgrade_thymio2_endpoint(std::string path, ranges::span<std::byte> firmware, uint16_t id,
                              firmware_upgrade_callback cb);
#endif

#ifdef MOBSYA_TDM_ENABLE_USB
void upgrade_thymio2_endpoint(libusb_device_handle* d, ranges::span<std::byte> firmware, uint16_t id,
                              firmware_upgrade_callback cb);
#endif

}  // namespace mobsya