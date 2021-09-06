#include "interfaces.h"
#include <boost/predef.h>
#include <bitset>
#include <boost/multiprecision/integer.hpp>

#if BOOST_OS_WINDOWS
#    include <iphlpapi.h>
#else
#    include <ifaddrs.h>
#endif


namespace mobsya {

static boost::asio::ip::address address_from_socket(sockaddr* sa) {
    char buf[200];
    if(sa->sa_family == AF_INET) {
        inet_ntop(AF_INET, (void*)&((struct sockaddr_in*)sa)->sin_addr, buf, sizeof(buf));
        return boost::asio::ip::make_address(buf);
    } else if(sa->sa_family == AF_INET6) {
        inet_ntop(AF_INET6, (void*)&((struct sockaddr_in6*)sa)->sin6_addr, buf, sizeof(buf));
        return boost::asio::ip::make_address(buf);
    }
    return {};
}

static boost::asio::ip::address v6netmask_from_prefix(uint32_t length) {
    std::array<unsigned char, 16> netmask;
    for (int32_t i = length, j = 0; i > 0; i -= 8, ++j)
      netmask[j] = i >= 8 ? 0xff : (uint32_t)(( 0xffU << ( 8 - i ) ) & 0xffU );
    return boost::asio::ip::address_v6(netmask);
}

#if BOOST_OS_WINDOWS

std::map<boost::asio::ip::address, boost::asio::ip::address> network_interfaces_addresses() {
    std::map<boost::asio::ip::address, boost::asio::ip::address> addresses;
    IP_ADAPTER_ADDRESSES staticBuf[3];  // 3 is arbitrary
    PIP_ADAPTER_ADDRESSES pAdapter = staticBuf;
    ULONG bufSize = sizeof staticBuf;

    ULONG flags = GAA_FLAG_INCLUDE_PREFIX | GAA_FLAG_SKIP_DNS_SERVER | GAA_FLAG_SKIP_MULTICAST;
    ULONG retval = GetAdaptersAddresses(AF_UNSPEC, flags, NULL, pAdapter, &bufSize);
    if(retval == ERROR_BUFFER_OVERFLOW) {
        // need more memory
        pAdapter = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(malloc(bufSize));
        if(!pAdapter)
            return {};
        // try again
        if(GetAdaptersAddresses(AF_UNSPEC, flags, NULL, pAdapter, &bufSize) != ERROR_SUCCESS) {
            free(pAdapter);
            return {};
        }
    } else if(retval != ERROR_SUCCESS) {
        // error
        return {};
    }

    // iterate over the list and add the entries to our listing
    for(PIP_ADAPTER_ADDRESSES ptr = pAdapter; ptr; ptr = ptr->Next) {
        // parse the IP (unicast) addresses
        for(PIP_ADAPTER_UNICAST_ADDRESS addr = ptr->FirstUnicastAddress; addr; addr = addr->Next) {
            // skip addresses in invalid state
            if(addr->DadState == IpDadStateInvalid)
                continue;
            auto sockaddr = addr->Address.lpSockaddr;
            if(!sockaddr)
                continue;
            auto address = address_from_socket(sockaddr);

            if(address.is_v4()) {
                ULONG MaskSize = addr->OnLinkPrefixLength;
                ULONG maskInt;
                ConvertLengthToIpv4Mask(MaskSize, &maskInt);
                auto netmask = boost::asio::ip::make_address_v4(maskInt);
                addresses.emplace(address, netmask);

            }
            if(address.is_v6()) {
                auto mask =  v6netmask_from_prefix(MaskSize).to_v6();
                addresses.emplace(address, mask);
                if(address.to_v6().is_v4_mapped() && mask.is_v4_mapped()) {
                    auto v4 = boost::asio::ip::make_address_v4(boost::asio::ip::v4_mapped_t{}, address.to_v6());
                    auto v4Mask = boost::asio::ip::make_address_v4(boost::asio::ip::v4_mapped_t{}, mask);
                    addresses.emplace(v4, v4Mask);
                }
            }
        }
    }

    if(pAdapter != staticBuf)
        free(pAdapter);

    return addresses;
}

#else

std::map<boost::asio::ip::address, boost::asio::ip::address> network_interfaces_addresses() {
    ifaddrs* lst;
    std::map<boost::asio::ip::address, boost::asio::ip::address> addresses;

    if(getifaddrs(&lst) == -1) {
        return {};
    }
    for(ifaddrs* ptr = lst; ptr; ptr = ptr->ifa_next) {
        if(!ptr->ifa_addr || !ptr->ifa_netmask)
            continue;
        auto address = address_from_socket(ptr->ifa_addr);
        auto mask    = address_from_socket(ptr->ifa_netmask);
        if(!address.is_unspecified() && !mask.is_unspecified()) {
            addresses.emplace(address, mask);

            if(address.is_v6() && address.to_v6().is_v4_mapped() &&
                    mask.is_v6() && mask.to_v6().is_v4_mapped()) {
                auto v4 = boost::asio::ip::make_address_v4(boost::asio::ip::v4_mapped_t{}, address.to_v6());
                auto v4Mask = boost::asio::ip::make_address_v4(boost::asio::ip::v4_mapped_t{}, address.to_v6());
                addresses.emplace(v4, v4Mask);
            }
        }
    }
    freeifaddrs(lst);
    return addresses;
}

#endif

bool endpoint_is_local(const boost::asio::ip::tcp::endpoint& ep) {
    return address_is_local(ep.address());
}

bool endpoint_is_network_local(const boost::asio::ip::tcp::endpoint& ep) {
    return address_is_local(ep.address());
}

bool address_is_local(const boost::asio::ip::address& addr) {
    auto local_ips = mobsya::network_interfaces_addresses();
    auto res = local_ips.find(addr) != local_ips.end();
    if(!res && addr.is_v6() && addr.to_v6().is_v4_mapped()) {
        auto v4 = boost::asio::ip::make_address_v4(boost::asio::ip::v4_mapped_t{}, addr.to_v6());
        res = local_ips.find(v4) != local_ips.end();
    }
    return res;
}

static bool check_mask(const boost::asio::ip::address_v4&  netip,
                       const boost::asio::ip::address_v4&  mask,
                       const boost::asio::ip::address_v4&  ip) {
    return ((netip.to_ulong() & mask.to_ulong()) == (ip.to_ulong() & mask.to_ulong()));
}

static bool check_mask(const boost::asio::ip::address_v6& netip,
                       const boost::asio::ip::address_v6&  mask,
                       const boost::asio::ip::address_v6&  ip) {

    using ipv6 = boost::multiprecision::uint128_t;

    // Convert to int128 integer to simplify mask checking
    auto from_buffer = [](const auto & buffer) -> ipv6 {
        boost::multiprecision::uint128_t ret = -1;
        memset(&ret, 0, 16);
        memcpy(&ret, buffer.data(), buffer.size());
        return ret;
    };

    ipv6 netip_int = from_buffer(netip.to_bytes());
    ipv6 mask_int = from_buffer(mask.to_bytes());
    ipv6 ip_int = from_buffer(ip.to_bytes());
    return ((netip_int & mask_int) == (ip_int & mask_int));
}

bool address_is_network_local(const boost::asio::ip::address &ip) {
    auto local_ips = mobsya::network_interfaces_addresses();
    for(auto & [netip, mask] : local_ips) {
        if(netip.is_v4() && ip.is_v4() && check_mask(netip.to_v4(), mask.to_v4(), ip.to_v4()))
            return true;
        if(netip.is_v6() && ip.is_v6() && check_mask(netip.to_v6(), mask.to_v6(), ip.to_v6()))
            return true;
    }
    return false;
}

}  // namespace mobsya
