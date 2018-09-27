#pragma once
#include <boost/asio/ip/address.hpp>
#include <set>

namespace mobsya {

std::set<boost::asio::ip::address> network_interfaces_addresses();

}