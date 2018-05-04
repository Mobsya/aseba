add_executable(switch
    main.cpp
    usbserver.cpp
    usbserver.h
    usbservice.cpp
    usbservice.h
    usbdevice.cpp
    usbdevice.h
    log.h
    log.cpp
    libusb_utils.h
    error.cpp
    error.h
)
target_link_libraries(switch boost::program_options boost::asio libusb spdlog tl_expected)
