# Core libraries
add_subdirectory(common)
add_subdirectory(compiler)
add_subdirectory(clients)

if(NOT ANDROID)
    add_subdirectory(vm)
    add_subdirectory(transport)

    add_subdirectory(targets)
    add_subdirectory(thymio-device-manager)
    add_subdirectory(qt-thymio-dm-client-lib)
    add_subdirectory(launcher)
endif()

add_subdirectory(flatbuffers)
