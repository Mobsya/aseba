
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

qt5_add_resources(QT_RESOURCES qml.qrc)
add_executable(thymio-launcher
    ${QT_RESOURCES}
    main.cpp
)
target_link_libraries(thymio-launcher
    Qt5::Widgets
    Qt5::Qml
    Qt5::Quick
    qt-thymio-dm-client-lib
)
install(TARGETS thymio-launcher RUNTIME DESTINATION bin)
codesign(thymio-launcher)
target_compile_definitions(thymio-launcher PRIVATE -DQT_QML_DEBUG)
