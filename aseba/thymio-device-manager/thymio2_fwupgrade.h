#pragma once
#include <fstream>
#include <range/v3/to_container.hpp>
#include "aseba_message_writer.h"
#include "aseba_message_parser.h"
#include "utils.h"
#include "log.h"
#include <sys/file.h>
#include "libusb/libusb.h"

namespace mobsya {

using firmware_data = std::vector<std::pair<uint32_t, std::vector<uint8_t>>>;

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

    template <typename Callback>
    struct upgrade_thymio2_endpoint_state {
        upgrade_thymio2_endpoint_state(boost::asio::io_service& service, uint16_t id, boost::asio::serial_port&& port,
                                       Callback&& cb, std::vector<std::pair<uint32_t, std::vector<uint8_t>>> pages)
            : io_context(service)
            , id(id)
            , ep(std::forward<boost::asio::serial_port>(port))
            , completion_cb(std::forward<Callback>(cb))
            , pages(std::move(pages))
            , timer(io_context) {}

        boost::asio::io_service& io_context;
        uint16_t id;
        boost::asio::serial_port ep;
        Callback completion_cb;
        std::vector<std::pair<uint32_t, std::vector<uint8_t>>> pages;
        std::vector<std::pair<uint32_t, std::vector<uint8_t>>>::const_iterator it;
        boost::asio::deadline_timer timer;
    };

    template <typename State>
    void do_upgrade_thymio2_endpoint_send_bootloader_reset(std::shared_ptr<State> state) {
        mLogInfo("[Firmware update] Reset Bootloader {}", state->id);
        auto msg = std::make_shared<Aseba::BootloaderReset>(0);
        mobsya::async_write_aseba_message(state->ep, *msg, [msg, state](boost::system::error_code ec) {
            mLogInfo("[Firmware update] {} : Done", state->id);
            state->completion_cb();
        });
    }

    /*template <typename State>
    void wait_read(std::shared_ptr<State> state) {
        uint16_t buffer[4] = {};
        boost::system::error_code ec;
        do {
            ::read(buffer, )
            mLogTrace("{}, {:x}{:x}{:x}{:x}", ec, buffer[0], buffer[1], buffer[2], buffer[3]);
            ec = {};
        } while(errno || (buffer[0] == 0 && buffer[1] == 0 && buffer[2] == 0 && buffer[3] == 0));
        mLogTrace("{}, {:x}{:x}{:x}{:x}", ec, buffer[0], buffer[1], buffer[2], buffer[3]);
    }*/

    inline void send_page(boost::asio::serial_port& p, int dest, int page, const std::vector<uint8_t>& data,
                          boost::system::error_code& ec) {
        mLogInfo("[Firmware update] {} : Sending page {}", dest, page);
        Aseba::BootloaderWritePage m(dest);
        m.source = 0;
        m.pageNumber = page;
        m.data = data;
        mobsya::write_aseba_message(p, m, ec);
        if(ec) {
            mLogTrace("Write error {} ({})", ec, ec.message());
            return;
        }
        uint16_t buffer[4] = {};
        int s = 0;
        mLogTrace("Wait ack{}", page);
        s = ::read(p.native_handle(), &buffer, 8);
        if(s == 0 || errno) {
            ec = boost::asio::error::basic_errors::timed_out;
            mLogTrace("{}({}), {:x}{:x}{:x}{:x}", s, errno, buffer[0], buffer[1], buffer[2], buffer[3]);
            return;
        }
        mLogTrace("{}({}), {:x}{:x}{:x}{:x}", s, errno, buffer[0], buffer[1], buffer[2], buffer[3]);
    }

    inline int write(int fd, const void* data, const size_t size) {
        assert(fd >= 0);

        if(size == 0)
            return 0;

        const char* ptr = (const char*)data;
        size_t left = size;

        while(left) {
            ssize_t len = ::write(fd, ptr, left);

            if(len < 0) {
                return errno;
            } else if(len == 0) {
                return errno;
            } else {
                ptr += len;
                left -= len;
            }
        }
    }

    /*template <typename State>
    void do_upgrade_thymio2_endpoint_send_page_step(std::shared_ptr<State> state) {
        if(state->it == state->pages.cend()) {
            do_upgrade_thymio2_endpoint_send_bootloader_reset(state);
            return;
        }
        mLogInfo("[Firmware update] {} : Sending page {}/{} - {}", state->id,
                 distance(state->pages.cbegin(), state->it), state->pages.size(), state->it->first);
        auto msg = std::make_shared<Aseba::BootloaderWritePage>(state->id);
        msg->source = 0;
        msg->pageNumber = state->it->first;
        msg->data = state->it->second;

        boost::system::error_code ec;
        mobsya::write_aseba_message(state->ep, *msg, ec);
        mLogInfo("[Firmware update] {} : Written for page {}/{} : {}", state->id,
                 distance(state->pages.cbegin(), state->it), state->pages.size(), ec);
        wait_read(state);
    }*/

    template <typename State>
    void do_upgrade_thymio2_endpoint(std::shared_ptr<State> state) {
        /*if(state->id == 0) {
            state->it = state->pages.begin();
            do_upgrade_thymio2_endpoint_send_page_step(std::move(state));
            return;
        }*/


        auto rebooot_msg = std::make_shared<Aseba::Reboot>(state->id);
        mLogInfo("[Firmware update] Rebooting {}", state->id);
        mobsya::async_write_aseba_message(state->ep, *rebooot_msg, [rebooot_msg, state](boost::system::error_code ec) {
            if(ec) {
                mLogError("Write error");
                return;
            }
            state->timer.expires_from_now(boost::posix_time::milliseconds(20));
            state->timer.async_wait([state](boost::system::error_code ec) {
                state->it = state->pages.begin();
                do_upgrade_thymio2_endpoint_send_page_step(std::move(state));
            });
        });
    }


}  // namespace details


inline int make_serial_device(boost::asio::serial_port& p, std::string path) {
    boost::system::error_code ec;
    p.close(ec);
    /*if(ec) {
        return errno;
    }*/
    auto fd = open(path.c_str(), O_RDWR);
    if(fd == -1)
        return errno;
    /*if(::flock(fd, LOCK_EX | LOCK_NB) != 0) {
        close(fd);
        return errno;
    }*/
    struct termios oldtio, newtio;
    tcgetattr(fd, &oldtio);
    memset(&newtio, 0, sizeof(newtio));

    newtio.c_cflag |= CLOCAL;  // ignore modem control lines.
    newtio.c_cflag |= CREAD;   // enable receiver.
    newtio.c_cflag |= CS8;
    newtio.c_cflag |= B1152000;
    newtio.c_iflag = IGNPAR;  // ignore parity on input
    newtio.c_oflag = 0;
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 0;  // block forever if no byte
    newtio.c_cc[VMIN] = 1;   // one byte is sufficient to return
    if(tcflush(fd, TCIOFLUSH) < 0 || (tcsetattr(fd, TCSANOW, &newtio) < 0)) {
        close(fd);
        return errno;
    }
    tcsetattr(fd, TCSANOW, &newtio);
    int flags = TIOCM_DTR;
    ioctl(fd, TIOCMBIS, &flags);
    p.assign(fd);
    return 0;
}


void upgrade_thymio2_endpoint(const firmware_data& data, uint16_t id);

}  // namespace mobsya

/*{

    libusb_context* ctx;
    int x = libusb_init(&ctx);
    auto d = libusb_open_device_with_vid_pid(ctx, 0x0617, 0x000a);
    if(!d) {
        return;
    }
    // libusb_device_handle* h;
    // auto res = libusb_open(d, &h);


    boost::asio::serial_port p(io_context);
    int idx = 0;
    while(true) {
        auto res = make_serial_device(p, port);
        if(res != 0) {
            mLogTrace("Opening error {} ...", res);
            continue;
        }
        // Wait for the device to be close
        boost::system::error_code ec;
        char buffer[100];
        while(true) {
            mLogTrace("Waiting reboot....");
            boost::system::error_code ec;
            int s = ::read(p.native_handle(), &buffer, 100);
            if(s == 0 || errno)
                break;
        }
        do {
            res = make_serial_device(p, port);
            if(res != 0) {
                // mLogTrace("Opening error...");
                continue;
            }
        } while(res != 0);
        // int flag = TIOCM_RTS;
        // ioctl(p.native_handle(), TIOCMBIS, &flag);
        // ioctl(p.native_handle(), TIOCMBIC, &flag);
        // std::this_thread::sleep_for(std::chrono::milliseconds(5));
        mLogTrace("Starting to write packets...");
        while(idx < data.size()) {
            mLogTrace("Sending page {}", idx);
            details::send_page(p, 1, data[idx].first, data[idx].second, ec);
            if(ec) {
                break;
            }
            idx++;
        }
    }

    // boost::asio::serial_port p(io_context);
    // p.assign(d->fd);

    // auto state = std::make_shared<details::upgrade_thymio2_endpoint_state<Callback>>(io_context, id,
    // std::move(p),
    //                                                                                 std::forward<Callback>(cb),
    //                                                                                 data);
    // details::do_upgrade_thymio2_endpoint(std::move(state));
}  // namespace mobsya
}  // namespace mobsya
*/