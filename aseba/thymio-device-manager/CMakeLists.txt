set(CMAKE_CXX_STANDARD 17)
add_executable(thymio-device-manager
    main.cpp
    log.h
    log.cpp
    error.cpp
    error.h
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
)

if(WIN32)
    target_sources(thymio-device-manager PRIVATE
        serialacceptor.h
        serialacceptor.cpp
        serialserver.h
        serialserver.cpp
        serial_usb_device.h
    )
    target_compile_definitions(thymio-device-manager PUBLIC MOBSYA_TDM_ENABLE_SERIAL)
	target_link_libraries(thymio-device-manager iphlpapi)
else()
    target_sources(thymio-device-manager PRIVATE
        usbserver.cpp
        usbserver.h
        usbacceptor.h
        usbacceptor.cpp
        usbdevice.cpp
        usbdevice.h
        usbcontext.h
    )
    target_compile_definitions(thymio-device-manager PUBLIC MOBSYA_TDM_ENABLE_USB)
endif()


target_compile_definitions(thymio-device-manager PUBLIC -DSPDLOG_FMT_EXTERNAL)
target_link_libraries(thymio-device-manager Boost::program_options Boost::filesystem Boost::asio Boost::date_time spdlog tl_expected asebacommon asebacompiler aware thymio-flatbuffers fmt-header-only)
if(WIN32)
    target_link_libraries(thymio-device-manager bcrypt)
else()
    target_link_libraries(thymio-device-manager libusb)
endif()

if (MSVC)
    target_compile_options(thymio-device-manager PRIVATE /bigobj)
endif()

install(TARGETS thymio-device-manager RUNTIME DESTINATION bin)
codesign(thymio-device-manager)
