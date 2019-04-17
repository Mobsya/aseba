#include "thymio2_fwupgrade.h"
#include <fstream>
#include "libusb/libusb.h"

namespace mobsya {
namespace details::libusb_firmware {
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
}  // namespace details::libusb_firmware

void upgrade_thymio2_endpoint(const thymio2_firmware_data& data, uint16_t id) {
    libusb_context* ctx;
    int x = libusb_init(&ctx);
    auto d = details::libusb_firmware::open_libusb_device(ctx);
    reset(d);
    int ms = 0;
    bool do_reset = true;
    int page = 0;
    while(1) {
        mLogInfo("LOOP");
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
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
    details::libusb_firmware::free_device(d);
    libusb_close(d.h);
}

}  // namespace mobsya