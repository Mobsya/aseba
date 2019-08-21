#pragma once
#include <boost/asio/io_service.hpp>
#include <boost/asio/use_future.hpp>
#include <boost/thread/future.hpp>
#include <vector>
#include <range/v3/span.hpp>

#include "log.h"
#include "aseba_node.h"
#include "aseba_node_registery.h"
#include "thymio2_fwupgrade.h"

namespace OB::Belle {
class Client;
}

namespace mobsya {


class firmware_update_service : public boost::asio::detail::service_base<firmware_update_service>,
                                public node_status_monitor {

public:
    firmware_update_service(boost::asio::execution_context& ctx);
    ~firmware_update_service() override;

    boost::unique_future<ranges::span<std::byte>> firmware_data(mobsya::aseba_node::node_type);

protected:
    void node_changed(std::shared_ptr<aseba_node>, const aseba_node_registery::node_id&, aseba_node::status) override;

private:
    boost::asio::execution_context& m_ctx;
    std::unique_ptr<OB::Belle::Client> m_http_client;
    void start();
    void download_thymio_2_firmware();
    void update_nodes_versions(mobsya::aseba_node::node_type);
    void download_firmare_info(mobsya::aseba_node::node_type type);
    void download_firmare_data(mobsya::aseba_node::node_type type);
    std::set<mobsya::aseba_node::node_type> m_downloading;
    std::map<mobsya::aseba_node::node_type, int> m_versions;
    std::map<mobsya::aseba_node::node_type, std::string> m_urls;
    std::map<mobsya::aseba_node::node_type, std::vector<std::byte>> m_firmwares_data;
    std::map<mobsya::aseba_node::node_type, std::vector<boost::promise<ranges::span<std::byte>>>> m_waiting;
};


}  // namespace mobsya
