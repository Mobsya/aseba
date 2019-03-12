#include "fw_update_service.h"
#include <belle/belle.hh>
#include <pugixml.hpp>

namespace belle = OB::Belle;

namespace mobsya {

static constexpr const auto UPDATE_SERVER = "www.mobsya.org";
static constexpr const auto THYMIO2_CHECK_PATH = "/update/Thymio2-firmware-meta.xml";

firmware_update_service::firmware_update_service(boost::asio::execution_context& ctx)
    : boost::asio::detail::service_base<firmware_update_service>(static_cast<boost::asio::io_context&>(ctx))
    , m_ctx(ctx)
    , m_http_client(std::make_unique<belle::Client>(UPDATE_SERVER, 443, true)) {

    start();
}

firmware_update_service::~firmware_update_service() {}

void firmware_update_service::start() {
    start_node_monitoring(boost::asio::use_service<aseba_node_registery>(m_ctx));
}

void firmware_update_service::node_changed(std::shared_ptr<aseba_node> node, const aseba_node_registery::node_id&,
                                           aseba_node::status status) {
    if(!node || status != aseba_node::status::available)
        return;
    auto type = node->type();
    if(type == aseba_node::node_type::Thymio2Wireless)
        type = aseba_node::node_type::Thymio2;
    auto it = m_versions.find(type);
    if(it != m_versions.end()) {
        update_nodes_versions(type);
        return;
    }

    if(type == aseba_node::node_type::Thymio2) {
        download_thymio_2_firmware();
        return;
    }
}

void firmware_update_service::download_thymio_2_firmware() {
    m_http_client->on_http_error([](auto& ctx) { mLogWarn("Http error", ctx.ec.message()); });
    mLogInfo("Downloading https://{}/{}", UPDATE_SERVER, THYMIO2_CHECK_PATH);
    m_http_client->on_http(THYMIO2_CHECK_PATH, [this](auto& ctx) {
        if(ctx.res.result() != belle::Status::ok) {
            mLogWarn("Http request failed: https://{}/{}", UPDATE_SERVER, THYMIO2_CHECK_PATH);
            return;
        }
        pugi::xml_document doc;
        pugi::xml_parse_result result = doc.load_string(ctx.res.body().c_str());
        auto node = doc.child("firmware");
        if(node) {
            auto v = node.attribute("version").as_int(0);
            if(v != 0) {
                m_versions[aseba_node::node_type::Thymio2] = v;
                update_nodes_versions(aseba_node::node_type::Thymio2);
                mLogInfo("Last firmware available for Thymio 2: {}", v);
            }
        }
    });
    m_http_client->connect();
}

void firmware_update_service::update_nodes_versions(mobsya::aseba_node::node_type type) {
    auto it = m_versions.find(type);
    if(it == m_versions.end())
        return;

    auto& registery = boost::asio::use_service<aseba_node_registery>(m_ctx);
    auto nodes = registery.nodes();
    for(auto ptr : nodes) {
        if(auto n = ptr.second.lock()) {
            auto node_type = n->type();
            if(node_type == aseba_node::node_type::Thymio2Wireless)
                node_type = aseba_node::node_type::Thymio2;
            if(type != node_type)
                continue;
            boost::asio::post([n, v = it->second] { n->set_available_firmware_version(v); });
        }
    }
}

}  // namespace mobsya