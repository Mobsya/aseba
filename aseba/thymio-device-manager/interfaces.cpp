#include "interfaces.h"
#include <boost/predef.h>

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

#if BOOST_OS_WINDOWS

std::set<boost::asio::ip::address> network_interfaces_addresses() {
    std::set<boost::asio::ip::address> addresses;
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
            if(!address.is_unspecified()) {
                addresses.insert(address);
                if(address.is_v6() && address.to_v6().is_v4_mapped()) {
                    auto v4 = boost::asio::ip::make_address_v4(boost::asio::ip::v4_mapped_t{}, address.to_v6());
                    addresses.insert(v4);
                }
            }
        }
    }

    if(pAdapter != staticBuf)
        free(pAdapter);

    return addresses;
}

#else

std::set<boost::asio::ip::address> network_interfaces_addresses() {
    ifaddrs* lst;
    std::set<boost::asio::ip::address> addresses;
    if(getifaddrs(&lst) == -1) {
        return {};
    }
    for(ifaddrs* ptr = lst; ptr; ptr = ptr->ifa_next) {
        if(!ptr->ifa_addr)
            continue;
        auto address = address_from_socket(ptr->ifa_addr);
        if(!address.is_unspecified()) {
            addresses.insert(address);
            if(address.is_v6() && address.to_v6().is_v4_mapped()) {
                auto v4 = boost::asio::ip::make_address_v4(boost::asio::ip::v4_mapped_t{}, address.to_v6());
                addresses.insert(v4);
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
bool address_is_local(const boost::asio::ip::address& addr) {
    std::set<boost::asio::ip::address> local_ips = mobsya::network_interfaces_addresses();
    auto res = local_ips.find(addr) != local_ips.end();
    if(!res && addr.is_v6() && addr.to_v6().is_v4_mapped()) {
        auto v4 = boost::asio::ip::make_address_v4(boost::asio::ip::v4_mapped_t{}, addr.to_v6());
        res = local_ips.find(v4) != local_ips.end();
    }
    return res;
}

}  // namespace mobsya
