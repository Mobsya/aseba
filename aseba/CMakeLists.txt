# Core libraries
add_subdirectory(common)
add_subdirectory(compiler)
add_subdirectory(clients)

if(NOT ANDROID)
    add_subdirectory(vm)
    add_subdirectory(transport)

    # User programs
    add_subdirectory(switches)
    add_subdirectory(targets)
endif()
