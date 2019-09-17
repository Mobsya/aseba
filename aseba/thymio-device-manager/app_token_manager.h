#pragma once
#include <boost/asio/io_service.hpp>
#include <boost/beast/core/span.hpp>
#include <vector>

namespace mobsya {

class app_token_manager : public boost::asio::detail::service_base<app_token_manager> {

public:
    using token = std::vector<uint8_t>;
    using token_view = boost::beast::span<const uint8_t>;
    app_token_manager(boost::asio::execution_context& ctx)
        : boost::asio::detail::service_base<app_token_manager>(static_cast<boost::asio::io_context&>(ctx)) {}

    bool check_token(token_view) const {
        return false;
    }

private:
};


}  // namespace mobsya