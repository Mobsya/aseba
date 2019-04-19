#include "thymio2_fwupgrade.h"
#include "log.h"
#include "range/v3/span.hpp"
#ifdef __linux__
#    include <linux/usbdevice_fs.h>
#endif
#ifdef __APPLE__
#    include <CoreFoundation/CoreFoundation.h>
#    include <IOKit/IOKitLib.h>
#    include <IOKit/serial/IOSerialKeys.h>
#endif

#include <boost/asio.hpp>
#include <boost/endian/conversion.hpp>
#include <aseba/common/msg/msg.h>

namespace mobsya {
namespace details {
    namespace {

#if defined(__APPLE__) or defined(__linux__)

        boost::system::error_code set_data_terminal_ready(boost::asio::serial_port::native_handle_type handle,
                                                          bool dtr) {
            int flag = TIOCM_DTR;
            auto r = ioctl(handle, dtr ? TIOCMBIS : TIOCMBIC, &flag);
            if(r == 0)
                return {};
            return boost::system::error_code{static_cast<int>(errno), boost::system::system_category()};
        }

        boost::system::error_code set_rts(boost::asio::serial_port::native_handle_type handle, bool rts) {
            int flag = TIOCM_RTS;
            auto r = ioctl(handle, rts ? TIOCMBIS : TIOCMBIC, &flag);
            if(r == 0)
                return {};
            return boost::system::error_code{static_cast<int>(errno), boost::system::system_category()};
        }

        boost::system::error_code reset_usb(boost::asio::serial_port::native_handle_type handle) {
#ifdef __APPLE__
#    define USBDEVFS_RESET _IO('U', 20)
#endif
            auto r = ioctl(handle, USBDEVFS_RESET, 0);
            if(r == 0)
                return {};
            return boost::system::error_code{static_cast<int>(errno), boost::system::system_category()};
        }

        boost::system::error_code flush(boost::asio::serial_port::native_handle_type handle) {
            std::this_thread::sleep_for(std::chrono::microseconds(50));
            auto r = 0;
#    ifdef __APPLE__
            r = tcdrain(handle);
#    endif
            if(r == 0)
                r = tcflush(handle, TCIOFLUSH);
            if(r == 0)
                return {};
            return boost::system::error_code{static_cast<int>(errno), boost::system::system_category()};
        }

        boost::system::error_code reset_device(boost::asio::serial_port::native_handle_type handle) {
            // reset_usb(handle);

            struct termios newtio;
            memset(&newtio, 0, sizeof(newtio));

            newtio.c_cflag |= CLOCAL;  // ignore modem control lines.
            newtio.c_cflag |= CREAD;   // enable receiver.
            newtio.c_cflag |= CS8;
#    ifdef __APPLE__
            cfsetspeed(&newtio, 1152000);
#    else
            newtio.c_cflag |= B1152000;
#    endif
            newtio.c_iflag = IGNPAR;  // ignore parity on input
            newtio.c_oflag = 0;
            newtio.c_lflag = 0;
            newtio.c_cc[VTIME] = 1;
            newtio.c_cc[VMIN] = 1;
            if(tcflush(handle, TCIOFLUSH) < 0 || (tcsetattr(handle, TCSANOW, &newtio) < 0)) {
                return boost::system::error_code{static_cast<int>(errno), boost::system::system_category()};
            }
            set_data_terminal_ready(handle, true);
            flush(handle);
            return {};
        }

        tl::expected<boost::asio::serial_port::native_handle_type, boost::system::error_code>
        make_serial_device(std::string path) {
            auto fd = open(path.c_str(), O_RDWR);
            if(fd == -1)
                return tl::make_unexpected(
                    boost::system::error_code{static_cast<int>(errno), boost::system::system_category()});
            if(::flock(fd, LOCK_EX | LOCK_NB) != 0) {
                close(fd);
                return tl::make_unexpected(
                    boost::system::error_code{static_cast<int>(errno), boost::system::system_category()});
            }
            reset_device(fd);
            return fd;
        }

        boost::system::error_code close(boost::asio::serial_port::native_handle_type h) {
            ::close(h);
            return {};
        }

        boost::system::error_code write(boost::asio::serial_port::native_handle_type h,
                                        ranges::span<unsigned char> data, int& written) {
            written = ::write(h, data.data(), data.size());
            if(written >= 1)
                return {};
            return boost::system::error_code{static_cast<int>(errno), boost::system::system_category()};
        }

        boost::system::error_code read(boost::asio::serial_port::native_handle_type h, ranges::span<unsigned char> data,
                                       int& read, int timeout_ms = 100) {

            read = 0;
            fd_set set;
            struct timeval timeout;

            FD_ZERO(&set);
            FD_SET(h, &set);

            timeout.tv_sec = 0;
            timeout.tv_usec = timeout_ms * 1000;

            int rv = select(h + 1, &set, NULL, NULL, &timeout);
            if(rv == -1) {
                return boost::system::error_code{static_cast<int>(errno), boost::system::system_category()};
            }
            if(rv == 0) {
                return boost::system::errc::make_error_code(boost::system::errc::timed_out);
            }

            read = ::read(h, data.data(), data.size());
            if(read >= 1)
                return {};
            return boost::system::error_code{static_cast<int>(errno), boost::system::system_category()};
        }
#endif  // __APPLE__ or __linux__
#ifdef _WIN32

        boost::system::error_code set_data_terminal_ready(boost::asio::serial_port::native_handle_type handle,
                                                          bool dtr) {
            if(!EscapeCommFunction(handle, dtr ? SETDTR : CLRDTR))
                return boost::system::error_code{static_cast<int>(GetLastError()), boost::system::system_category()};
            return {};
        }

        boost::system::error_code set_rts(boost::asio::serial_port::native_handle_type handle, bool rts) {
            if(!EscapeCommFunction(handle, rts ? SETRTS : CLRRTS))
                return boost::system::error_code{static_cast<int>(GetLastError()), boost::system::system_category()};
            return {};
        }

        // No windows implementation
        boost::system::error_code reset_usb(boost::asio::serial_port::native_handle_type handle) {
            return {};
        }

        boost::system::error_code flush(boost::asio::serial_port::native_handle_type handle) {
            if(!FlushFileBuffers(handle))
                return boost::system::error_code{static_cast<int>(GetLastError()), boost::system::system_category()};
            return {};
        }

        boost::system::error_code reset_device(boost::asio::serial_port::native_handle_type handle) {

            DCB dcb;
            memset(&dcb, 0, sizeof(dcb));
            dcb.DCBlength = sizeof(dcb);
            if(!GetCommState(handle, &dcb)) {
                return boost::system::error_code{static_cast<int>(GetLastError()), boost::system::system_category()};
            }
            dcb.DCBlength = sizeof(dcb);
            dcb.fOutxCtsFlow = FALSE;
            dcb.fRtsControl = RTS_CONTROL_DISABLE;
            dcb.fOutxDsrFlow = FALSE;
            dcb.fDtrControl = DTR_CONTROL_ENABLE;
            dcb.fDsrSensitivity = FALSE;
            dcb.fBinary = TRUE;
            dcb.fParity = TRUE;
            dcb.BaudRate = 1152000;
            dcb.ByteSize = 8;
            dcb.Parity = NOPARITY;
            dcb.StopBits = ONESTOPBIT;


            if(!SetCommState(handle, &dcb)) {
                return boost::system::error_code{static_cast<int>(GetLastError()), boost::system::system_category()};
            }


            COMMTIMEOUTS cto;

            if(!GetCommTimeouts(handle, &cto)) {
                return boost::system::error_code{static_cast<int>(GetLastError()), boost::system::system_category()};
            }

            cto.ReadIntervalTimeout = 10;
            cto.ReadTotalTimeoutConstant = 100;
            cto.ReadTotalTimeoutMultiplier = 1;
            cto.WriteTotalTimeoutConstant = 100;
            cto.WriteTotalTimeoutMultiplier = 1;


            if(!SetCommTimeouts(handle, &cto)) {
                return boost::system::error_code{static_cast<int>(GetLastError()), boost::system::system_category()};
            }

            set_data_terminal_ready(handle, true);
            flush(handle);
            return {};
        }

        tl::expected<boost::asio::serial_port::native_handle_type, boost::system::error_code>
        make_serial_device(std::string path) {
            auto handle = CreateFileA(path.c_str(), GENERIC_WRITE | GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
            if(handle == nullptr) {
                return tl::make_unexpected(
                    boost::system::error_code{static_cast<int>(GetLastError()), boost::system::system_category()});
            }
            reset_device(handle);
            return handle;
        }

        boost::system::error_code close(boost::asio::serial_port::native_handle_type h) {
            if(CloseHandle(h))
                return {};
            return boost::system::error_code{static_cast<int>(GetLastError()), boost::system::system_category()};
        }

        boost::system::error_code write(boost::asio::serial_port::native_handle_type h,
                                        ranges::span<unsigned char> data, unsigned long& written) {
            auto r = WriteFile(h, data.data(), data.size(), &written, nullptr);
            if(!r) {
                return boost::system::error_code{static_cast<int>(errno), boost::system::system_category()};
            }
            return {};
        }

        boost::system::error_code read(boost::asio::serial_port::native_handle_type h, ranges::span<unsigned char> data,
                                       unsigned long& read, int timeout_ms = 100) {

            if(!ReadFile(h, data.data(), data.size(), &read, nullptr)) {
                return boost::system::error_code{static_cast<int>(GetLastError()), boost::system::system_category()};
            }
            return {};
        }
#endif


        boost::system::error_code write(boost::asio::serial_port::native_handle_type handle,
                                        const Aseba::Message& msg) {
            Aseba::Message::SerializationBuffer buffer;
            buffer.add(uint16_t{0});
            buffer.add(msg.source);
            buffer.add(msg.type);
            msg.serializeSpecific(buffer);
            uint16_t& size = *(reinterpret_cast<uint16_t*>(buffer.rawData.data()));
            size = boost::endian::native_to_little(static_cast<uint16_t>(buffer.rawData.size()) - 6);
            unsigned long res_size = 0;
            auto err = write(handle, buffer.rawData, res_size);
            flush(handle);
            return err;
        }

        boost::system::error_code read_ack(boost::asio::serial_port::native_handle_type handle) {
            uint8_t buffer[8] = {};
            unsigned long s = 0;
            if(auto err = read(handle, buffer, s)) {
                return err;
            }
            if(s == 8) {
                auto msg_type = boost::endian::little_to_native(*reinterpret_cast<uint16_t*>(buffer + 4));
                if(msg_type == ASEBA_MESSAGE_BOOTLOADER_ACK) {
                    auto err = boost::endian::little_to_native(*reinterpret_cast<uint16_t*>(buffer + 6));
                    mLogInfo("ASEBA_MESSAGE_BOOTLOADER_ACK: 0x{:x}", err);
                    return {};
                }
            }
            return boost::system::errc::make_error_code(boost::system::errc::no_message);
        }

        uint16_t get_node_id(boost::asio::serial_port::native_handle_type handle) {
            write(handle, Aseba::GetDescription{});
            if(auto err = write(handle, Aseba::ListNodes{}))
                return 0;
            uint8_t buffer[64] = {};
            unsigned long s = 0;
            if(auto err = read(handle, buffer, s, 100)) {
                mLogError("List nodes {}", err.message());
                return 0;
            }
            if(s < 6)
                return 0;
            auto msg_source = boost::endian::little_to_native(*reinterpret_cast<uint16_t*>(buffer + 2));
            return msg_source;
        }

        boost::system::error_code send_page(boost::asio::serial_port::native_handle_type handle, int dest, int page,
                                            const std::vector<uint8_t>& data) {
            mLogInfo("[Firmware update] {} : Sending page {}", dest, page);
            Aseba::BootloaderWritePage m(dest);
            m.pageNumber = page;
            write(handle, m);
            auto total = 0;
            while(total < data.size()) {
                auto s = std::min(int(data.size()), 64);
                unsigned long res_size = 0;
                auto err = write(handle, ranges::span(((unsigned char*)data.data() + total), s), res_size);
                total += res_size;
                if(err) {
                    mLogInfo("Writting chunk {}/{} failed: {}", total, data.size(), err.message());
                    return err;
                }
                flush(handle);
            }
            mLogTrace("Wait ack {}", page);
            auto err = read_ack(handle);
            if(err) {
                mLogTrace("Ack failed : {}", err.message());
                return err;
            }
            return {};
        }

        tl::expected<boost::asio::serial_port::native_handle_type, boost::system::error_code>
        try_make_serial_device(std::string path) {
            auto h = make_serial_device(path);
            int i = 0;
            while(!h.has_value()) {
                // mLogError("Opening device failed: {}", h.error().message());
                if(i > 1000)
                    break;
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
                h = make_serial_device(path);
                i++;
            }
            if(!h.has_value()) {
                mLogError("Opening device failed: {}", h.error().message());
            }
            return h;
        }

    }  // namespace

    void upgrade_thymio2_endpoint(std::string path, const thymio2_firmware_data& data, uint16_t id,
                                  firmware_upgrade_callback cb) {
        int page = 0;

        // On OSX it is important to wait between close and open
        // And this function might have been called right after
        // the device was closed
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        auto h = try_make_serial_device(path);
        if(!h.has_value()) {
            cb(h.error(), 0, false);
            return;
        }

        // Give a chance to the device to boot up
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        if(id == 0) {
            mLogTrace("Trying to get the node id...");
            id = get_node_id(*h);
            mLogInfo("Node Id is now {}", id);
        }

        mLogInfo("Reboot...");
        write(*h, Aseba::Reboot(id));

        reset_device(*h);

        // Give a chance to the device to enter the bootloader
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        uint8_t buffer[64] = {};
        unsigned long s = 0;
        auto err = read(*h, buffer, s);
        mLogInfo("Reading: {} - {}\n", s, err.message());

        do {
            mLogInfo("Sending pages...");
            for(; page < data.size(); page++) {
                mLogInfo("Sending page {}", data.size() - page - 1);
                if(send_page(*h, 1, data[data.size() - page - 1].first, data[data.size() - page - 1].second)) {
                    mLogError("Fail to send page {}", data.size() - page - 1);
                    break;
                }
                cb({}, page / double(data.size()), false);
            }
            if(page == data.size()) {
                mLogInfo("Sending reset");
                write(*h, Aseba::BootloaderReset{});
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                reset_device(*h);
                close(*h);
                cb({}, 1, true);
                return;
            }

            close(*h);
            // On OSX it is important to wait between close and open
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            h = h = try_make_serial_device(path);
            if(!h.has_value()) {
                cb(h.error(), 0, false);
                return;
            }
        } while(true);

        cb({}, 1, false);  // Should never happen
    }
}  // namespace details
}  // namespace mobsya
