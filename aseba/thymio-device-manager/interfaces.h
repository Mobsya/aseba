#pragma once
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <set>

namespace mobsya {

std::set<boost::asio::ip::address> network_interfaces_addresses();
bool endpoint_is_local(const boost::asio::ip::tcp::endpoint& ep);
bool address_is_local(const boost::asio::ip::address& addr);

}  // namespace mobsya