set(CMAKE_AUTOMOC ON)
find_package(Qt5 COMPONENTS Core Widgets Network REQUIRED)

add_library(qtlockedfile STATIC
    qt-solutions/qtlockedfile/src/qtlockedfile.cpp
    qt-solutions/qtlockedfile/src/QtLockedFile
    qt-solutions/qtlockedfile/src/qtlockedfile.cpp
    qt-solutions/qtlockedfile/src/qtlockedfile.h   
)

if(WIN32)
    target_sources(qtlockedfile PRIVATE qt-solutions/qtlockedfile/src/qtlockedfile_win.cpp)
else()
    target_sources(qtlockedfile PRIVATE qt-solutions/qtlockedfile/src/qtlockedfile_unix.cpp)
endif()
target_link_libraries(qtlockedfile Qt5::Core)
target_include_directories(qtlockedfile PUBLIC qt-solutions/qtlockedfile/src/)


add_library(qtsingleapplication STATIC
    qt-solutions/qtsingleapplication/src/qtlocalpeer.cpp
    qt-solutions/qtsingleapplication/src/qtlocalpeer.h
    qt-solutions/qtsingleapplication/src/QtSingleApplication
    qt-solutions/qtsingleapplication/src/qtsingleapplication.cpp
    qt-solutions/qtsingleapplication/src/qtsingleapplication.h
    qt-solutions/qtsingleapplication/src/qtsinglecoreapplication.cpp
    qt-solutions/qtsingleapplication/src/qtsinglecoreapplication.h
)
target_link_libraries(qtsingleapplication qtlockedfile Qt5::Network Qt5::Widgets)
target_include_directories(qtsingleapplication PUBLIC qt-solutions/qtsingleapplication/src/)