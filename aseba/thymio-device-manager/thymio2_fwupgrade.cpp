#include "thymio2_fwupgrade.h"
#include <fstream>
#include "libusb/libusb.h"

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
            unsigned chunk_size = data.size();

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


    struct device {
        libusb_device* d;
        libusb_device_handle* h;
        int in;
        int out;
    };

    void free_device(device& d) {
        for(int if_num = 0; if_num < 2; if_num++) {
            if(!libusb_kernel_driver_active(d.h, if_num)) {
                auto r = libusb_attach_kernel_driver(d.h, if_num);
                if(r > 0 && r != LIBUSB_ERROR_NOT_SUPPORTED) {
                    return;
                }
            }
        }
    }

    void reset(device& d) {
        libusb_config_descriptor* desc = nullptr;
        if(auto r = libusb_get_active_config_descriptor(d.d, &desc)) {
            return;
        }
        libusb_reset_device(d.h);

        for(int if_num = 0; if_num < 2; if_num++) {
            if(libusb_kernel_driver_active(d.h, if_num)) {
                auto r = libusb_detach_kernel_driver(d.h, if_num);
                if(r > 0 && r != LIBUSB_ERROR_NOT_SUPPORTED) {
                    return;
                }
            }
            if((libusb_claim_interface(d.h, if_num))) {
                //  return;
            }
        }

        int in = 0;
        int out = 0;

        for(int i = 0; i < desc->bNumInterfaces; i++) {
            const auto interface = desc->interface[i];
            for(int s = 0; s < interface.num_altsetting; s++) {
                if(interface.altsetting[s].bInterfaceClass != LIBUSB_CLASS_DATA)
                    continue;
                libusb_claim_interface(d.h, i);
                for(int e = 0; e < interface.altsetting[s].bNumEndpoints; e++) {
                    const auto endpoint = interface.altsetting[s].endpoint[e];
                    if(endpoint.bmAttributes & LIBUSB_TRANSFER_TYPE_BULK) {
                        if((endpoint.bEndpointAddress & LIBUSB_ENDPOINT_IN) == LIBUSB_ENDPOINT_IN) {
                            in = endpoint.bEndpointAddress;
                        } else if((endpoint.bEndpointAddress & LIBUSB_ENDPOINT_OUT) == LIBUSB_ENDPOINT_OUT) {
                            out = endpoint.bEndpointAddress;
                        }
                    }
                }
            }
        }
        d.in = in;
        d.out = out;

        std::array<unsigned char, 7> control_line;
        constexpr int br = 115200;
        control_line[0] = uint8_t(br & 0xff);
        control_line[1] = uint8_t((br >> 8) & 0xff);
        control_line[2] = uint8_t((br >> 16) & 0xff);
        control_line[3] = uint8_t((br >> 24) & 0xff);
        control_line[4] = 1;

        libusb_control_transfer(d.h, 0x21, 0x22, 0x00, 0, nullptr, 0, 0);
        libusb_control_transfer(d.h, 0x21, 0x22, 0x02, 0, nullptr, 0, 0);
        libusb_control_transfer(d.h, 0x21, 0x22, 0x01, 0, nullptr, 0, 0);
        libusb_control_transfer(d.h, 0x21, 0x20, 0, 0, control_line.data(), control_line.size(), 0);
        libusb_clear_halt(d.h, in);
        libusb_clear_halt(d.h, out);
    }


    device open_libusb_device(libusb_context* ctx) {
        auto h = libusb_open_device_with_vid_pid(ctx, 0x0617, 0x000a);
        if(!h) {
            return {};
        }
        libusb_device* d = libusb_get_device(h);
        device dev;
        dev.h = h;
        dev.d = d;
        reset(dev);
        return dev;
    }


    bool usb_write_aseba_message(device& d, const Aseba::Message& msg) {
        Aseba::Message::SerializationBuffer buffer;
        buffer.add(uint16_t{0});
        buffer.add(msg.source);
        buffer.add(msg.type);
        msg.serializeSpecific(buffer);
        uint16_t& size = *(reinterpret_cast<uint16_t*>(buffer.rawData.data()));
        size = boost::endian::native_to_little(static_cast<uint16_t>(buffer.rawData.size()) - 6);
        int res_size = 0;

        int err = 0;
        while((err = libusb_bulk_transfer(d.h, d.out, buffer.rawData.data(), buffer.rawData.size(), &res_size, 200)) !=
              0) {
            ;
            return false;
        }
        return true;
    }

    inline bool send_page(device& d, int dest, int page, const std::vector<uint8_t>& data) {
        mLogInfo("[Firmware update] {} : Sending page {}", dest, page);
        Aseba::BootloaderWritePage m(dest);
        m.pageNumber = page;
        // m.data = data;
        usb_write_aseba_message(d, m);
        auto total = 0;
        while(total < data.size()) {
            auto s = std::min(int(data.size()), 64);
            int res_size = 0;
            int err = libusb_bulk_transfer(d.h, d.out, ((unsigned char*)data.data() + total), s, &res_size, 300);
            total += res_size;
            if(err != 0)
                return false;
            // mLogInfo("Written : {}/{} ({})", total, data.size(), err);
        }

        uint8_t buffer[64] = {};
        int s = 0;
        mLogTrace("Wait ack{}", page);
        auto err = libusb_bulk_transfer(d.h, d.in, buffer, 64, &s, 100);
        if(err != 0) {
            return false;
        }

        mLogInfo("Reading: {} - {}({})\n", s, libusb_strerror(libusb_error(err)), err);
        if(s >= 6) {
            auto msg_size = boost::endian::little_to_native(*reinterpret_cast<uint16_t*>(buffer));
            auto msg_source = boost::endian::little_to_native(*reinterpret_cast<uint16_t*>(buffer + 2));
            auto msg_type = boost::endian::little_to_native(*reinterpret_cast<uint16_t*>(buffer + 4));
            mLogInfo("Msg received : size : {} source : {} type: {:x}", msg_size, msg_source, msg_type);
            if(msg_type == ASEBA_MESSAGE_BOOTLOADER_ACK) {
                auto err = boost::endian::little_to_native(*reinterpret_cast<uint16_t*>(buffer + 6));
                mLogInfo("ASEBA_MESSAGE_BOOTLOADER_ACK: 0x{:x}", err);
                return true;
            }
        }
        return false;
    }
}  // namespace details

void upgrade_thymio2_endpoint(const firmware_data& data, uint16_t id) {
    libusb_context* ctx;
    int x = libusb_init(&ctx);
    auto d = details::open_libusb_device(ctx);
    reset(d);
    int ms = 0;
    bool do_reset = true;
    int page = 0;
    while(1) {
        mLogInfo("LOOP");
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        auto d = details::open_libusb_device(ctx);
        reset(d);
        if(!d.h) {
            continue;
        }
        mLogInfo("Reboot...");
        usb_write_aseba_message(d, Aseba::Reboot(id));
        reset(d);
        uint8_t buffer[64] = {};
        int s = 0;
        auto err = libusb_bulk_transfer(d.h, d.in, buffer, 64, &s, 100);
        mLogInfo("Reading: {} - {}({})\n", s, libusb_strerror(libusb_error(err)), err);

        if(d.h) {
            for(; page < data.size(); page++) {
                mLogInfo("Sending page {}", page);
                if(!send_page(d, 1, data[page].first, data[page].second)) {
                    mLogError("Fail to send page {}", page);
                    break;
                }
            }
            if(page == data.size()) {
                mLogInfo("Sending reset");
                usb_write_aseba_message(d, Aseba::BootloaderReset{});
                std::this_thread::sleep_for(std::chrono::seconds(5));
                int i = 0;
                while(i < 2) {
                    auto d = details::open_libusb_device(ctx);
                    reset(d);
                    mLogInfo("Reboot");
                    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                    usb_write_aseba_message(d, Aseba::Reboot{id});
                    i++;
                }
                break;
            }
            libusb_close(d.h);
        }
        // std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    details::free_device(d);
    libusb_close(d.h);
}

}  // namespace mobsya