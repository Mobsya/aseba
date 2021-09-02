#pragma once
#include <boost/asio/io_service.hpp>
#include <boost/beast/core/span.hpp>
#include <vector>
#include <string>
#include <string_view>
#include <random>


namespace mobsya {

class app_token_manager : public boost::asio::detail::service_base<app_token_manager> {

public:
    using token = std::vector<uint8_t>;
    using token_view = boost::beast::span<const uint8_t>;
    app_token_manager(boost::asio::execution_context& ctx)
        : boost::asio::detail::service_base<app_token_manager>(static_cast<boost::asio::io_context&>(ctx))
        , tdm_password(generate_pasword()){}

    bool check_token(token_view) const {
        return false;
    }

    bool check_tdm_password(std::string_view password) const {
        return tdm_password.empty() || password == tdm_password;
    }

    const std::string & password() const {
        return tdm_password;
    }

private:
    std::string generate_pasword() const {
        std::string chars("ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890");
        std::random_device r;
        std::mt19937 gen(r());
        std::uniform_int_distribution<std::size_t> dist(0, chars.size());
        std::string out;
        for(int i = 0; i < 6; i++) {
            out.push_back(chars[dist(gen)]);
        }
        return out;
    }

    std::string tdm_password;
};


}  // namespace mobsya
