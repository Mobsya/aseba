#include "thymio2_fwupgrade.h"
#include <fstream>
#include <range/v3/algorithm.hpp>

namespace mobsya {
namespace details {
    unsigned get_uint4(std::istream& stream) {
        int c = stream.get();
        if(c <= '9')
            return c - '0';
        else if(c <= 'F')
            return c - 'A' + 10;
        else
            return c - 'a' + 10;
    }

    unsigned get_uint8(std::istream& stream) {
        return (get_uint4(stream) << 4) | get_uint4(stream);
    }

    unsigned get_uint16(std::istream& stream) {
        return (get_uint8(stream) << 8) | get_uint8(stream);
    }

    tl::expected<chunks, hex_file_parse_error> read_hex_file(std::string hex_data) {
        std::stringstream ifs(hex_data);

        if(ifs.bad())
            return tl::make_unexpected(hex_file_parse_error::file_error);

        int line_counter = 0;
        uint32_t baseAddress = 0;
        chunks data;

        while(ifs.good()) {
            // leading ":" character
            char c;
            ifs >> c;
            if(c != ':')
                return tl::make_unexpected(hex_file_parse_error::invalid_record);

            uint8_t computedCheckSum = 0;

            // record data length
            uint8_t dataLength = get_uint8(ifs);
            computedCheckSum += dataLength;

            // short address
            uint16_t lowAddress = get_uint16(ifs);
            computedCheckSum += lowAddress;
            computedCheckSum += lowAddress >> 8;

            // record type
            uint8_t recordType = get_uint8(ifs);
            computedCheckSum += recordType;

            switch(recordType) {
                case 0: {  // data record
                    // read data
                    std::vector<uint8_t> recordData;
                    for(int i = 0; i != dataLength; i++) {
                        uint8_t d = get_uint8(ifs);
                        computedCheckSum += d;
                        recordData.push_back(d);
                    }

                    // verify checksum
                    uint8_t checkSum = get_uint8(ifs);
                    computedCheckSum = 1 + ~computedCheckSum;
                    if(checkSum != computedCheckSum)
                        return tl::make_unexpected(hex_file_parse_error::wrong_checksum);

                    // compute address
                    uint32_t address = lowAddress;
                    address += baseAddress;
                    // std::cout << "data record at address 0x" << std::hex << address << "\n";

                    // is some place to add
                    bool found = false;
                    for(auto& it : data) {
                        size_t chunkSize = it.second.size();
                        if(address == it.first + chunkSize) {
                            // copy new
                            std::copy(recordData.begin(), recordData.end(), std::back_inserter(it.second));
                            found = true;
                            // std::cout << "tail fusable chunk found\n";
                            break;
                        } else if(address + recordData.size() == it.first) {
                            // resize
                            it.second.resize(chunkSize + recordData.size());
                            // move
                            std::copy_backward(it.second.begin(), it.second.begin() + chunkSize, it.second.end());
                            // copy new
                            std::copy(recordData.begin(), recordData.end(), it.second.begin());
                            found = true;
                            // std::cout << "head fusable chunk found\n";
                            break;
                        }
                    }
                    if(!found)
                        data[address] = recordData;
                } break;

                case 1: {  // end of file record

                    for(auto it = data.begin(); it != data.end(); ++it) {
                        // std::cout << "End of file found. Address " << it->first << " size " <<
                        // it->second.size() << "\n";
                    }
                    return data;
                    break;
                }

                case 2: {  // extended segment address record
                    if(dataLength != 2)
                        return tl::make_unexpected(hex_file_parse_error::invalid_record);

                    // read data
                    uint16_t highAddress = get_uint16(ifs);
                    computedCheckSum += highAddress;
                    computedCheckSum += highAddress >> 8;
                    baseAddress = highAddress;
                    baseAddress <<= 4;
                    // std::cout << "Extended segment address record (?!): 0x" << std::hex <<
                    // baseAddress << "\n";

                    // verify checksum
                    uint8_t checkSum = get_uint8(ifs);
                    computedCheckSum = 1 + ~computedCheckSum;
                    if(checkSum != computedCheckSum)
                        return tl::make_unexpected(hex_file_parse_error::invalid_record);
                    break;
                }

                case 4: {  // extended linear address record
                    if(dataLength != 2)
                        return tl::make_unexpected(hex_file_parse_error::invalid_record);
                    // read data
                    uint16_t highAddress = get_uint16(ifs);
                    computedCheckSum += highAddress;
                    computedCheckSum += highAddress >> 8;
                    baseAddress = highAddress;
                    baseAddress <<= 16;
                    // std::cout << "Linear address record: 0x" << std::hex << baseAddress << "\n";

                    // verify checksum
                    uint8_t checkSum = get_uint8(ifs);
                    computedCheckSum = 1 + ~computedCheckSum;
                    if(checkSum != computedCheckSum)
                        return tl::make_unexpected(hex_file_parse_error::wrong_checksum);
                    break;
                }

                case 5: {  // start linear address record
                    // Start linear address record are not used by the Aseba
                    // bootloader protocol so we can ignore them.

                    // Skip data
                    for(int i = 0; i < 4; i++)
                        computedCheckSum += get_uint8(ifs);

                    computedCheckSum = 1 + ~computedCheckSum;

                    uint8_t checkSum = get_uint8(ifs);

                    if(checkSum != computedCheckSum)
                        return tl::make_unexpected(hex_file_parse_error::wrong_checksum);
                }

                break;


                default: return tl::make_unexpected(hex_file_parse_error::unknown_record);
            }

            line_counter++;
        }

        return tl::make_unexpected(hex_file_parse_error::file_truncated);
    }

    tl::expected<page_map, hex_file_parse_error> extract_pages_from_hex(std::string hex_data) {
        auto res = read_hex_file(hex_data);
        if(!res.has_value())
            return tl::make_unexpected(res.error());

        const auto& chunks = res.value();
        page_map pages;
        auto page_size = 2048;

        for(auto&& [address, data] : chunks) {
            // index inside data chunk
            unsigned chunkDataIndex = 0;
            // size of chunk in bytes
            unsigned chunk_size = unsigned(data.size());

            do {
                // get page number
                unsigned pageIndex = (address + chunkDataIndex) / page_size;
                // get address inside page
                unsigned byteIndex = (address + chunkDataIndex) % page_size;

                // if page does not exists, create it
                if(pages.find(pageIndex) == pages.end()) {
                    pages.emplace(pageIndex, std::vector<uint8_t>(page_size, uint8_t(0)));
                }
                // copy data
                unsigned amountToCopy = std::min(page_size - byteIndex, chunk_size - chunkDataIndex);
                std::copy(data.begin() + chunkDataIndex, data.begin() + chunkDataIndex + amountToCopy,
                          pages[pageIndex].begin() + byteIndex);

                // increment chunk data pointer
                chunkDataIndex += amountToCopy;
            } while(chunkDataIndex < chunk_size);
        }
        return pages;
    }

    template <typename F>
    void do_upgrade_thymio2_endpoint(ranges::span<std::byte> firmware, F&& f, uint16_t id, firmware_upgrade_callback cb,
                                     firmware_update_options options) {
        std::string str;
        str.reserve(firmware.size());
        ranges::transform(firmware, ranges::back_inserter(str), [](std::byte b) { return char(b); });
        auto pages = mobsya::details::extract_pages_from_hex(str);
        if(!pages.has_value()) {
            cb(boost::system::errc::make_error_code(boost::system::errc::bad_file_descriptor), 0, 0);
            return;
        }
        mobsya::thymio2_firmware_data data(pages.value().begin(), pages.value().end());
        f(std::move(data), std::move(cb), id, options);

        //
    }
}  // namespace details

#ifdef MOBSYA_TDM_ENABLE_SERIAL
void upgrade_thymio2_endpoint(std::string path, ranges::span<std::byte> firmware, uint16_t id,
                              firmware_upgrade_callback cb, firmware_update_options options) {

    auto f = [path](const thymio2_firmware_data& data, firmware_upgrade_callback cb, uint16_t id,
                    firmware_update_options options) {
        details::upgrade_thymio2_serial_endpoint(path, data, id, std::move(cb), options);
    };

    std::thread t(details::do_upgrade_thymio2_endpoint<decltype(f)>, firmware, f, id, std::move(cb), options);
    t.detach();
}
#endif

#ifdef MOBSYA_TDM_ENABLE_USB
void upgrade_thymio2_endpoint(libusb_device_handle* d, ranges::span<std::byte> firmware, uint16_t id,
                              firmware_upgrade_callback cb, firmware_update_options options) {
    auto f = [d](const thymio2_firmware_data& data, firmware_upgrade_callback cb, uint16_t id,
                 firmware_update_options options) {
        details::upgrade_thymio2_usb_endpoint(d, data, id, std::move(cb), options);
    };

    std::thread t(details::do_upgrade_thymio2_endpoint<decltype(f)>, firmware, f, id, std::move(cb), options);
    t.detach();
}
#endif

}  // namespace mobsya