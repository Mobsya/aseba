#pragma once
#include <boost/asio/io_service.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <vector>
#include <boost/bind/bind.hpp>
#include <boost/chrono.hpp>

#ifdef __APPLE__
#    include <IOKit/pwr_mgt/IOPMLIb.h>
#endif
#ifdef WIN32
#include <windows.h>
#endif

namespace mobsya {

class system_sleep_manager : public boost::asio::detail::service_base<system_sleep_manager> {

public:
    system_sleep_manager(boost::asio::execution_context& ctx)
        : boost::asio::detail::service_base<system_sleep_manager>(static_cast<boost::asio::io_context&>(ctx)) {}

    ~system_sleep_manager() {
        on_last_app_disconnected();
    }


    void app_connected() {
        m_app_count++;

        if(m_app_count == 1) {
            on_first_app_connected();
        }
    }

    void app_disconnected() {
        m_app_count--;

        if(m_app_count == 0) {
            on_last_app_disconnected();
        }
    }

    void on_first_app_connected() {
#ifdef __APPLE__
        CFStringRef reasonForActivity = CFSTR("Thymio device manager");
        IOReturn success = IOPMAssertionCreateWithName(kIOPMAssertionTypeNoIdleSleep, kIOPMAssertionLevelOn,
                                                       reasonForActivity, &m_iokit_assertion_id);
#endif
#ifdef WIN32
        SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED);
#endif
    }

    void on_last_app_disconnected() {
#ifdef __APPLE__
        IOPMAssertionRelease(m_iokit_assertion_id);
#endif
#ifdef WIN32
        SetThreadExecutionState(ES_CONTINUOUS);
#endif
    }


private:
    int m_app_count = 0;
#ifdef __APPLE__
    IOPMAssertionID m_iokit_assertion_id;
#endif
};


}  // namespace mobsya
