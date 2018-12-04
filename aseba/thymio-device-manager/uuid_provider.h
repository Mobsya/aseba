#pragma once
#include <boost/asio/io_service.hpp>
#include "node_id.h"

namespace mobsya {

class uuid_generator : public boost::asio::detail::service_base<uuid_generator> {
public:
    uuid_generator(boost::asio::execution_context& io_context)
        : boost::asio::detail::service_base<uuid_generator>(static_cast<boost::asio::io_context&>(io_context)) {}
    boost::uuids::uuid generate() {
        return m_generator();
    }

private:
    boost::uuids::random_generator m_generator{};
};

}  // namespace mobsya