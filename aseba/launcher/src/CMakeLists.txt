set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
cmake_policy(SET CMP0071 NEW)

find_package(Qt5QuickCompiler)
if(Qt5QuickCompiler_FOUND)
qtquick_compiler_add_resources(QT_RESOURCES qml.qrc)
else()
    qt5_add_resources(QT_RESOURCES qml.qrc)
endif()

add_executable(thymio-launcher WIN32
    ${QT_RESOURCES}
    main.cpp
    launcher.h
    launcher.cpp
    tdmsupervisor.h
    tdmsupervisor.cpp
)
target_link_libraries(thymio-launcher
    Qt5::Widgets
    Qt5::Qml
    Qt5::Quick
    Qt5::QuickWidgets
    qt-thymio-dm-client-lib
    qtsingleapplication
)
codesign(thymio-launcher)
install_qt_app(thymio-launcher)
target_compile_definitions(thymio-launcher PRIVATE -DQT_QML_DEBUG)
