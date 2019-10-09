# Core libraries
add_subdirectory(common)
add_subdirectory(compiler)
add_subdirectory(clients)

if(NOT ANDROID AND NOT IOS)
     add_subdirectory(targets)
     add_subdirectory(thymio-device-manager)
     add_subdirectory(vm)
     add_subdirectory(transport)
endif()

add_subdirectory(qt-thymio-dm-client-lib)
add_subdirectory(launcher)
add_subdirectory(flatbuffers)
