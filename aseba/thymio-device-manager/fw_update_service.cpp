#include "fw_update_service.h"
#define BOOST_PREDEF_DETAIL_ENDIAN_COMPAT_H
#include <belle/belle.hh>
#include <pugixml.hpp>
#include <range/v3/algorithm/transform.hpp>
#include <range/v3/span.hpp>

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

    download_firmare_info(type);
}

void firmware_update_service::download_firmare_info(mobsya::aseba_node::node_type type) {
    if(type == aseba_node::node_type::Thymio2) {
        download_thymio_2_firmware();
        return;
    }
}

void firmware_update_service::download_thymio_2_firmware() {
    m_http_client->on_http_error([](auto& ctx) { mLogWarn("Http error", ctx.ec.message()); });
    if(m_downloading.find(aseba_node::node_type::Thymio2) != m_downloading.end())
        return;
    m_downloading.insert(aseba_node::node_type::Thymio2);

    mLogInfo("Downloading https://{}/{}", UPDATE_SERVER, THYMIO2_CHECK_PATH);
    m_http_client->on_http(THYMIO2_CHECK_PATH, [this](auto& ctx) {
        m_downloading.erase(aseba_node::node_type::Thymio2);

        if(ctx.res.result() != belle::Status::ok) {
            mLogWarn("Http request failed: https://{}/{}", UPDATE_SERVER, THYMIO2_CHECK_PATH);
            return;
        }
        pugi::xml_document doc;
        pugi::xml_parse_result result = doc.load_string(ctx.res.body().c_str());
        if(result.status != pugi::xml_parse_status::status_ok) {
            mLogError("The firmware update manifest might be corrupted");
            return;
        }
        auto node = doc.child("firmware");
        if(node) {
            auto v = node.attribute("version").as_int(0);
            if(v != 0) {
                m_versions[aseba_node::node_type::Thymio2] = v;
                auto str = node.attribute("url").as_string();
                m_urls[aseba_node::node_type::Thymio2] = str;

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
            if(n->firwmware_version() != it->second)
                boost::asio::post([n, v = it->second] { n->set_available_firmware_version(v); });
        }
    }
    auto pit = m_waiting.find(type);
    if(pit != m_waiting.end() && pit->second.size() > 0)
        download_firmare_data(type);
}

boost::unique_future<ranges::span<std::byte>>
firmware_update_service::firmware_data(mobsya::aseba_node::node_type type) {
    auto p = boost::promise<ranges::span<std::byte>>();

    if(type == aseba_node::node_type::Thymio2Wireless)
        type = aseba_node::node_type::Thymio2;

    auto it = m_firmwares_data.find(type);
    if(it != m_firmwares_data.end()) {
        p.set_value(it->second);
        return p.get_future();
    }

    auto url_it = m_urls.find(type);
    if(url_it == m_urls.end()) {
        download_firmare_info(type);
        return p.get_future();
    }

    auto f = p.get_future();

    m_waiting[type].emplace_back(std::move(p));

    if(m_waiting[type].size() == 1)
        download_firmare_data(type);

    return f;
}

void firmware_update_service::download_firmare_data(mobsya::aseba_node::node_type type) {
    auto url_it = m_urls.find(type);
    if(url_it == m_urls.end()) {
        return;
    }

    auto cb = [type, this, url = url_it->second](auto& ctx) {
        mLogInfo("{} : {}", url, ctx.res.result());
        if(ctx.res.result() == belle::Status::ok) {
            std::vector<std::byte> v;
            v.reserve(ctx.res.body().size());
            ranges::transform(ctx.res.body(), ranges::back_inserter(v), [](char b) { return std::byte(b); });
            m_firmwares_data.emplace(type, v);

            // Notify clients
            for(auto&& p : m_waiting[type]) {
                p.set_value(m_firmwares_data[type]);
            }
            m_waiting.clear();
        }
    };

    m_http_client->on_http(url_it->second, std::move(cb));
    m_http_client->connect();
}  // namespace mobsya

}  // namespace mobsya
