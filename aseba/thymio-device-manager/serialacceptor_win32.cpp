#include "serialacceptor.h"
#include <algorithm>
#include <charconv>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/beast/core/bind_handler.hpp>
#include "log.h"

#include <initguid.h>
#include <devguid.h>   // for GUID_DEVCLASS_PORTS and GUID_DEVCLASS_MODEM
#include <winioctl.h>  // for GUID_DEVINTERFACE_COMPORT
#include <setupapi.h>
#include <cfgmgr32.h>
#include <boost/scope_exit.hpp>


namespace mobsya {

serial_acceptor_service::serial_acceptor_service(boost::asio::io_context& io_service)
    : boost::asio::detail::service_base<serial_acceptor_service>(io_service)
    , m_active_timer(io_service)
    , m_strand(io_service.get_executor()) {

    mLogInfo("Serial monitoring service: started");
}


void serial_acceptor_service::shutdown() {
    mLogInfo("Serial monitoring service: Stopped");
    m_active_timer.cancel();
}

void serial_acceptor_service::free_device(const std::string& s) {
    const auto it = std::find(m_known_devices.begin(), m_known_devices.end(), s);
    if(it != m_known_devices.end()) {
        mLogError("Removed {}", s);
        m_known_devices.erase(it);
    }
    remove_connected_device(s);
    on_active_timer({});
}


void serial_acceptor_service::register_request(request& r) {
    m_active_timer.expires_from_now(boost::posix_time::millisec(1));
    m_active_timer.async_wait(boost::asio::bind_executor(
        m_strand, boost::bind(&serial_acceptor_service::on_active_timer, this, boost::placeholders::_1)));
}

void serial_acceptor_service::on_active_timer(const boost::system::error_code& ec) {
    if(ec)
        return;
    std::unique_lock<std::mutex> lock(m_req_mutex);
    this->handle_request_by_active_enumeration();
    if(!m_requests.empty()) {
        m_active_timer.expires_from_now(boost::posix_time::seconds(1));  // :(
        m_active_timer.async_wait(boost::asio::bind_executor(
            m_strand, boost::bind(&serial_acceptor_service::on_active_timer, this, boost::placeholders::_1)));
    }
}

static std::string device_instance_identifier(DEVINST deviceInstanceNumber) {
    std::string str(MAX_DEVICE_ID_LEN + 1, 0);
    if(::CM_Get_Device_IDA(deviceInstanceNumber, &str[0], MAX_DEVICE_ID_LEN, 0) != CR_SUCCESS) {
        return {};
    }
    for(char& c : str)
        c = std::toupper(c, std::locale());
    return str;
}
static usb_device_identifier device_id_from_interface_id(const std::string& str) {
    auto value = [&str](const char* prefix) {
        uint16_t res = 0;
        if(auto needle = str.find(prefix); needle != std::string::npos) {
            auto start = needle + strlen(prefix);
            std::from_chars(str.data() + start, str.data() + str.size(), res, 16);
        }
        return res;
    };
    return {value("VID_"), value("PID_")};
}

static std::string get_device_name(HDEVINFO deviceInfoSet, PSP_DEVINFO_DATA deviceInfoData) {
    DWORD dataType = 0;
    std::string outputBuffer(MAX_PATH + 1, 0);
    DWORD bytesRequired = MAX_PATH;
    for(;;) {
        if(::SetupDiGetDeviceRegistryPropertyA(deviceInfoSet, deviceInfoData, SPDRP_DEVICEDESC, &dataType,
                                              reinterpret_cast<PBYTE>(outputBuffer.data()), bytesRequired,
                                              &bytesRequired)) {
            break;
        }

        if(::GetLastError() != ERROR_INSUFFICIENT_BUFFER || (dataType != REG_SZ && dataType != REG_EXPAND_SZ)) {
            return {};
        }
        outputBuffer.resize(bytesRequired);
    }
    return outputBuffer;
}


static std::string get_com_portname(HDEVINFO info_set, PSP_DEVINFO_DATA device_info) {
    const HKEY key = ::SetupDiOpenDevRegKey(info_set, device_info, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
    if(key == INVALID_HANDLE_VALUE) {
        mLogTrace("SetupDiOpenDevRegKey : {}-{} ", GetLastError(), get_last_win32_error_string());
        return {};
    }
    auto value = [&key](const char* subk) -> std::string {
        DWORD dataType = 0;
        std::string str(MAX_PATH + 1, 0);
        DWORD bytesRequired = MAX_PATH;
        for(;;) {
            const LONG ret =
                ::RegQueryValueExA(key, subk, nullptr, &dataType, reinterpret_cast<PBYTE>(&str[0]), &bytesRequired);
            if(ret == ERROR_MORE_DATA) {
                str.resize(bytesRequired / sizeof(str) + 1, 0);
                continue;
            } else if(ret == ERROR_SUCCESS) {
                if(dataType == REG_SZ) {
                    if(str.find("COM") == 0) {
                        return fmt::format("\\\\.\\{}", str);
                    }
                    return str;
                }
                else if(dataType == REG_DWORD)
                    return fmt::format("\\\\.\\COM{}", *(PDWORD(&str[0])));
            }
            return {};
        }
    };
    std::string port_name = value("PortName");
    if(port_name.empty())
        port_name = value("PortNumber");
    ::RegCloseKey(key);
    return port_name;
}

void serial_acceptor_service::handle_request_by_active_enumeration() {
    if(m_requests.empty() || m_paused)
        return;
    const auto devices = m_requests.front().acceptor.compatible_devices();


    const HDEVINFO deviceInfoSet = ::SetupDiGetClassDevs(&GUID_DEVCLASS_PORTS, nullptr, nullptr, DIGCF_PRESENT);
    if(deviceInfoSet == INVALID_HANDLE_VALUE)
        return;

    BOOST_SCOPE_EXIT(&deviceInfoSet) {
        ::SetupDiDestroyDeviceInfoList(deviceInfoSet);
    }
    BOOST_SCOPE_EXIT_END

    std::vector<std::string> known_devices;

    SP_DEVINFO_DATA deviceInfoData;
    ::memset(&deviceInfoData, 0, sizeof(deviceInfoData));
    deviceInfoData.cbSize = sizeof(deviceInfoData);



    DWORD index = 0;
    while(true) {
        int32_t res = ::SetupDiEnumDeviceInfo(deviceInfoSet, index++, &deviceInfoData);
        if(res == 0) {
            if(GetLastError() != ERROR_NO_MORE_ITEMS) {
                mLogTrace("SetupDiEnumDeviceInfo : {}-{} ", GetLastError(), get_last_win32_error_string());
            }
            break;
        }


        const auto str = device_instance_identifier(deviceInfoData.DevInst);
        const auto id = device_id_from_interface_id(str);

        if(std::find(std::begin(devices), std::end(devices), id) == std::end(devices)) {
            continue;
        }
        const auto port_name = get_com_portname(deviceInfoSet, &deviceInfoData);

        known_devices.push_back(port_name);
        if(m_requests.empty() ||
           std::find(m_connected_devices.begin(), m_connected_devices.end(), port_name) != m_connected_devices.end())
            continue;

        const auto device_name = get_device_name(deviceInfoSet, &deviceInfoData);

        auto& req = m_requests.front();
        mLogTrace("device : {:#06X}-{:#06X} on {}", id.vendor_id, id.product_id, port_name);
        boost::system::error_code ec;
        req.d.m_device_id = id;
        req.d.m_device_name = device_name;
        req.d.m_port_name = port_name;
        req.d.open(ec);
        if(!ec) {
            m_connected_devices.push_back(port_name);

            auto handler = std::move(req.handler);
            const auto executor = boost::asio::get_associated_executor(handler, req.acceptor.get_executor());
            m_requests.pop();
            boost::asio::post(executor, boost::beast::bind_handler(handler, boost::system::error_code{}));
        }
    }
    for(auto&& e : m_known_devices) {
        if(std::find(known_devices.begin(), known_devices.end(), e) == known_devices.end()) {
            mLogError("Removed {}", e);
            remove_connected_device(e);
            device_unplugged(e);
        }
    }
    m_known_devices = known_devices;
}


void serial_acceptor_service::construct(implementation_type&) {}
void serial_acceptor_service::destroy(implementation_type&) {}

serial_acceptor::serial_acceptor(boost::asio::io_context& io_service,
                                 std::initializer_list<usb_device_identifier> devices)
    : boost::asio::basic_io_object<serial_acceptor_service>(io_service) {

    std::copy(std::begin(devices), std::end(devices),
              std::back_inserter(this->get_implementation().compatible_devices));
}

const std::vector<usb_device_identifier>& serial_acceptor::compatible_devices() const {
    return this->get_implementation().compatible_devices;
}


}  // namespace mobsya
