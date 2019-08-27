#pragma once
#include <boost/asio/io_service.hpp>
#include <vector>
#include <range/v3/span.hpp>

#include "log.h"
#include "aseba_node.h"
#include "aseba_node_registery.h"
#include "tl/expected.hpp"

namespace mobsya {


class wireless_configurator_service : public boost::asio::detail::service_base<wireless_configurator_service> {

public:
    wireless_configurator_service(boost::asio::execution_context& ctx);
    ~wireless_configurator_service() override;

    bool is_enabled() const {
        return m_enabled;
    }

    void enable();

    void register_configurable_dongle(usb_serial_port&& d);
    void device_unplugged(std::string_view);

    struct dongle {
        usb_serial_port device;
        struct node_id uuid;

        uint16_t node_id = 0;
        uint16_t network_id = 0;
        uint16_t channel = 0;
    };

    std::vector<std::reference_wrapper<const dongle> > dongles() const;
    boost::signals2::signal<void()> wireless_dongles_changed;


    enum class configure_error { no_such_dongle, sync_failed };

    tl::expected<std::reference_wrapper<const dongle>, configure_error>
    configure_dongle(const node_id& n, uint16_t network_id, uint8_t channel);

private:
    bool m_enabled = false;
    void disconnect_all_nodes();
    boost::asio::io_context& m_ctx;


    bool sync(dongle& dongle, bool flash);

    PACK(struct dongle_data {
        unsigned short nodeId = 0xffff;
        unsigned short panId = 0xffff;
        unsigned char channel = 0xff;
        unsigned char txPower = 0;
        unsigned char version = 0;
        unsigned char ctrl = 0;
    });

    std::unordered_map<std::string, dongle> m_dongles;
};


}  // namespace mobsya
