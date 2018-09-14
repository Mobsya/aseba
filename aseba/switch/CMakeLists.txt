set(CMAKE_CXX_STANDARD 17)
add_executable(switch
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
)

if(WIN32)
    target_sources(switch PRIVATE
        serialacceptor.h
        serialacceptor.cpp
        serialserver.h
        serialserver.cpp
        serial_usb_device.h
    )
    target_compile_definitions(switch PUBLIC MOBSYA_TDM_ENABLE_SERIAL)
else()
    target_sources(switch PRIVATE
        usbserver.cpp
        usbserver.h
        usbacceptor.h
        usbacceptor.cpp
        usbdevice.cpp
        usbdevice.h
        usbcontext.h
    )
    target_compile_definitions(switch PUBLIC MOBSYA_TDM_ENABLE_USB)
endif()


target_compile_definitions(switch PUBLIC -DSPDLOG_FMT_EXTERNAL)
target_link_libraries(switch Boost::program_options Boost::filesystem Boost::asio Boost::date_time spdlog tl_expected asebacommon asebacompiler aware thymio-flatbuffers fmt-header-only)
if(WIN32)
    target_link_libraries(switch bcrypt)
else()
    target_link_libraries(switch libusb)
endif()

if (MSVC)
    target_compile_options(switch PRIVATE /bigobj)
endif()

install(TARGETS switch RUNTIME DESTINATION bin)
codesign(switch)
