find_package(Qt5 COMPONENTS Widgets Qml Quick QuickWidgets REQUIRED)
if(APPLE)
    find_package(Qt5 COMPONENTS WebView REQUIRED)
else()
    find_package(Qt5 COMPONENTS WebEngine REQUIRED)
endif()

add_subdirectory(src)
