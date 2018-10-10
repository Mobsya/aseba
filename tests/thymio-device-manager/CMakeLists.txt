set(CMAKE_CXX_STANDARD 17)

add_executable(tdm_tests
    runner.cpp
    aesl.cpp
    property.cpp
)
set_property(TARGET tdm_tests PROPERTY CXX_STANDARD 17)
target_link_libraries(tdm_tests catch2 thymio-device-manager-lib)
add_test(tdm_tests tdm_tests)