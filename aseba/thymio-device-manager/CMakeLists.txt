set(CMAKE_CXX_STANDARD 17)
add_library(thymio-device-manager-lib STATIC
    main.cpp
    log.h
    log.cpp
    error.cpp
    error.h
    events.h
    aesl_parser.h
    aesl_parser.cpp
    aseba_message_parser.h
    aseba_description_receiver.h
    aseba_message_writer.h
    aseba_node_registery.h
    aseba_node_registery.cpp
    aseba_endpoint.h
    aseba_node.h
    aseba_node.cpp
    aseba_tcpacceptor.h
    aseba_tcpacceptor.cpp
    app_server.h
    app_token_manager.h
    app_endpoint.h
    flatbuffers_message_reader.h
    flatbuffers_message_writer.h
    flatbuffers_messages.h
    node_id.h
    usb_utils.h
    utils.h
    tdm.h
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
        serialacceptor.cpp
        serialserver.h
        serialserver.cpp
        serial_usb_device.h
    )
    target_compile_definitions(thymio-device-manager-lib  PUBLIC MOBSYA_TDM_ENABLE_SERIAL)
    target_link_libraries(thymio-device-manager-lib PUBLIC iphlpapi)
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
endif()


target_compile_definitions(thymio-device-manager-lib PUBLIC -DSPDLOG_FMT_EXTERNAL)
target_link_libraries(thymio-device-manager-lib PUBLIC
    asebacommon
    asebacompiler
    thymio-flatbuffers
    Boost::program_options
    Boost::filesystem Boost::asio
    Boost::date_time
    spdlog
    tl_expected
    aware
    fmt-header-only
    mpark_variant
    pugixml
       )
if(WIN32)
    target_link_libraries(thymio-device-manager-lib PUBLIC bcrypt)
else()
    target_link_libraries(thymio-device-manager-lib PUBLIC libusb)
endif()

if (MSVC)
    target_compile_options(thymio-device-manager-lib PUBLIC /bigobj)
endif()

add_executable(thymio-device-manager main.cpp)
target_link_libraries(thymio-device-manager PUBLIC thymio-device-manager-lib)

install(TARGETS thymio-device-manager RUNTIME DESTINATION bin)
codesign(thymio-device-manager)
