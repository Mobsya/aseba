set(CMAKE_CXX_STANDARD 17)

add_executable(tst_thymio-device-manager
    runner.cpp
    aesl.cpp
    property.cpp
)
target_link_libraries(tst_thymio-device-manager PUBLIC catch2 thymio-device-manager-lib)
add_test(NAME tst_thymio-device-manager COMMAND tst_thymio-device-manager)