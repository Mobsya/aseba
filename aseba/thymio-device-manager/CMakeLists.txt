set(CMAKE_CXX_STANDARD 17)
find_package(OpenSSL REQUIRED)

add_library(thymio-device-manager-lib STATIC
    main.cpp
    log.h
    log.cpp
    common_types.h
    error.cpp
    error.h
    events.h
    aesl_parser.h
    aesl_parser.cpp
    aseba_message_parser.h
    aseba_message_writer.h
    aseba_node_registery.h
    aseba_node_registery.cpp
    aseba_nodeid_generator.h
    uuid_provider.h
    aseba_device.h
    aseba_device.cpp
    aseba_endpoint.h
    aseba_endpoint.cpp
    aseba_node.h
    aseba_node.cpp
    aseba_tcpacceptor.h
    aseba_tcpacceptor.cpp
    aseba_property.h
    app_server.h
    app_server.cpp
    app_token_manager.h
    app_endpoint.h
    app_endpoint.cpp
    flatbuffers_message_reader.h
    flatbuffers_message_writer.h
    flatbuffers_messages.h
    thymio2_fwupgrade.h
    thymio2_fwupgrade.cpp
    thymio2_fwupgrade_impl.cpp
    fw_update_service.h
    fw_update_service.cpp
    wireless_configurator_service.h
    wireless_configurator_service.cpp
    system_sleep_manager.h
    node_id.h
    usb_utils.h
    utils.h
    tdm.h
    group.h
    group.cpp
    interfaces.h
    interfaces.cpp
    property.h
    property_flexbuffer.h
    property_flexbuffer.cpp
    variant_compat.h
)

if(WIN32)
    target_sources(thymio-device-manager-lib PRIVATE
        serialacceptor.h
        serialacceptor_win32.cpp
        serialserver.h
        serialserver.cpp
        serial_usb_device.h
    )
    target_compile_definitions(thymio-device-manager-lib  PUBLIC MOBSYA_TDM_ENABLE_SERIAL)
    target_link_libraries(thymio-device-manager-lib PUBLIC iphlpapi CRYPT32)
elseif(APPLE)
    target_sources(thymio-device-manager-lib PRIVATE
        serialacceptor.h
        serialacceptor_osx.cpp
        serialserver.h
        serialserver.cpp
        serial_usb_device.h
    )
    target_compile_definitions(thymio-device-manager-lib  PUBLIC MOBSYA_TDM_ENABLE_SERIAL)
else()
    find_package(udev)
    if(UDEV_FOUND)
        target_sources(thymio-device-manager-lib PRIVATE
            serialacceptor.h
            serialacceptor_udev.cpp
            serialserver.h
            serialserver.cpp
            serial_usb_device.h
        )
        target_link_libraries(thymio-device-manager-lib PUBLIC ${UDEV_LIBRARIES})
        target_compile_definitions(thymio-device-manager-lib  PUBLIC MOBSYA_TDM_ENABLE_SERIAL MOBSYA_TDM_ENABLE_UDEV)
    else()
        target_sources(thymio-device-manager-lib PRIVATE
            usbserver.cpp
            usbserver.h
            usbacceptor.h
            usbacceptor.cpp
            usbdevice.cpp
            usbdevice.h
            usbcontext.h
        )
        target_compile_definitions(thymio-device-manager-lib PUBLIC MOBSYA_TDM_ENABLE_USB)
        target_link_libraries(thymio-device-manager-lib PUBLIC libusb)
    endif()
endif()

target_include_directories(thymio-device-manager-lib PUBLIC ${CMAKE_SOURCE_DIR}/third_party/belle/include)

target_compile_definitions(thymio-device-manager-lib PUBLIC
    -DSPDLOG_FMT_EXTERNAL
    -DBOOST_ALLOW_DEPRECATED_HEADERS
    -DBOOST_THREAD_PROVIDES_FUTURE_CONTINUATION
    -DBOOST_BEAST_ALLOW_DEPRECATED
    )
target_link_libraries(thymio-device-manager-lib PUBLIC
    asebacommon
    asebacompiler
    thymio-flatbuffers
    Boost::program_options
    Boost::filesystem Boost::asio
    Boost::date_time
    Boost::thread
    spdlog
    tl_expected
    aware
    fmt-header-only
    mpark_variant
    pugixml
    range-v3
    OpenSSL::SSL
    OpenSSL::Crypto)

if(APPLE)
    find_library(CoreFoundation CoreFoundation)
    find_library(IOKit IOKit)
	find_library(OpenSSL OpenSSL)
    target_link_libraries(thymio-device-manager-lib PUBLIC ${CoreFoundation} ${IOKit} ${OpenSSL})
endif()

if(WIN32)
    target_link_libraries(thymio-device-manager-lib PUBLIC bcrypt winspool ws2_32 setupapi)
endif()

if (MSVC)
    target_compile_options(thymio-device-manager-lib PUBLIC /bigobj)
endif()

add_executable(thymio-device-manager main.cpp)
target_link_libraries(thymio-device-manager PUBLIC thymio-device-manager-lib)

install(TARGETS thymio-device-manager RUNTIME DESTINATION bin)
codesign(thymio-device-manager)

add_executable(thymio2-firmware-upgrader thymio2_fwupgrade_standalone_main.cpp)
target_link_libraries(thymio2-firmware-upgrader PUBLIC thymio-device-manager-lib)

install(TARGETS thymio2-firmware-upgrader RUNTIME DESTINATION bin)
codesign(thymio2-firmware-upgrader)
