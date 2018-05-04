#pragma once
#include <libusb/libusb.h>
#include <mutex>
#include <memory>

namespace mobsya {

namespace details {
    class usb_context {
    public:
        usb_context() {
            libusb_init(&ctx);
        }
        using ptr = std::shared_ptr<usb_context>;

        ~usb_context() {
            libusb_exit(ctx);
        }
        usb_context(const usb_context&) = delete;
        usb_context(usb_context&& other) {
            ctx = other.ctx;
            other.ctx = nullptr;
        }
        operator libusb_context*() const {
            return ctx;
        }

        static std::shared_ptr<usb_context> acquire_context() {
            struct data {
                std::mutex m;
                std::weak_ptr<usb_context> wp;
            };

            static data d;
            std::unique_lock<std::mutex> lock(d.m);
            auto sp = d.wp.lock();
            if(sp)
                return sp;
            sp = std::make_shared<usb_context>();
            d.wp = sp;
            return sp;
        }

    private:
        libusb_context* ctx;
    };
}  // namespace details

}  // namespace mobsya
