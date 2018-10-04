set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_library(qt-thymio-dm-client-lib STATIC
    thymiodevicemanagerclient.cpp
    thymiodevicemanagerclient.h
    thymiodevicemanagerclientendpoint.cpp
    thymiodevicemanagerclientendpoint.h
    thymiodevicesmodel.cpp
    thymiodevicesmodel.h
    thymionode.cpp
    thymionode.h
    thymio-api.h
    qflatbuffers.h
)
target_link_libraries(qt-thymio-dm-client-lib
    Qt5::Core
    Qt5::Network
    QtZeroConf
    thymio-flatbuffers
)
target_include_directories(qt-thymio-dm-client-lib PUBLIC .)
