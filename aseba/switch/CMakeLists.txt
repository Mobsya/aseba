set(CMAKE_CXX_STANDARD 17)
add_executable(switch
    main.cpp
    usbserver.cpp
    usbserver.h
    usbacceptor.h
    usbacceptor.cpp
    usbdevice.cpp
    usbdevice.h
    usbcontext.h
    log.h
    log.cpp
    libusb_utils.h
    error.cpp
    error.h
    aseba_message_parser.h
    aseba_description_receiver.h
    aseba_message_writer.h
    aseba_endpoint.h
    aseba_node.h
    aseba_node.cpp
    dispatcher.h
)
target_link_libraries(switch boost::program_options boost::asio libusb spdlog tl_expected asebacommon)
