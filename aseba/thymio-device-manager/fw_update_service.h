#pragma once
#include <boost/asio/io_service.hpp>
#include <vector>
#include "log.h"
#include "aseba_node.h"
#include "aseba_node_registery.h"

namespace OB::Belle {
class Client;
}

namespace mobsya {


class firmware_update_service : public boost::asio::detail::service_base<firmware_update_service>,
                                public node_status_monitor {

public:
    firmware_update_service(boost::asio::execution_context& ctx);
    ~firmware_update_service();

protected:
    void node_changed(std::shared_ptr<aseba_node>, const aseba_node_registery::node_id&, aseba_node::status) override;

private:
    boost::asio::execution_context& m_ctx;
    std::unique_ptr<OB::Belle::Client> m_http_client;
    void start();
    void download_thymio_2_firmware();
    void update_nodes_versions(mobsya::aseba_node::node_type);
    std::map<mobsya::aseba_node::node_type, int> m_versions;
};


}  // namespace mobsya
