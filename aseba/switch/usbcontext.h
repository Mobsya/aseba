#pragma once
#include <libusb/libusb.h>
#include <mutex>
#include <memory>
#include <thread>
#include <vector>
#include <algorithm>
#include "log.h"

namespace mobsya {

namespace details {
    class usb_context {
    public:
        usb_context() {
            libusb_init(&ctx);
            m_thread = std::thread([this]() { run(); });
            // libusb_set_debug(ctx, LIBUSB_LOG_LEVEL_DEBUG);
        }
        using ptr = std::shared_ptr<usb_context>;

        ~usb_context() {
            m_running = false;
            if(m_thread.joinable())
                m_thread.join();
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

        bool is_device_open(libusb_device* d) const {
            std::unique_lock<std::mutex> _(m_device_mutex);
            return std::find(m_open_devices.begin(), m_open_devices.end(), d) != m_open_devices.end();
        }

        void mark_open(libusb_device* d) {
            std::unique_lock<std::mutex> _(m_device_mutex);
            m_open_devices.push_back(d);
        }

        void mark_not_open(libusb_device* d) {
            std::unique_lock<std::mutex> _(m_device_mutex);
            m_open_devices.erase(std::remove(m_open_devices.begin(), m_open_devices.end(), d), m_open_devices.end());
        }


    private:
        void run() {
            m_running = true;
            timeval tv{7, 0};
            while(m_running) {
                libusb_handle_events_timeout(ctx, &tv);
            }
        }

        libusb_context* ctx;
        std::thread m_thread;
        std::atomic_bool m_running;
        std::vector<libusb_device*> m_open_devices;
        mutable std::mutex m_device_mutex;
    };
}  // namespace details

}  // namespace mobsya
