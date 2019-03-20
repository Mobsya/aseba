if (ENKI_FOUND)
    add_executable(aseba-test-simulator aseba-test-simulator.cpp)
    target_link_libraries(aseba-test-simulator asebasim asebacompiler asebavm asebavmbuffer asebacommon enki Qt5::Core)

    # the following tests should succeed
    add_test(NAME robot-simulator-thymio COMMAND aseba-test-simulator)
endif()
