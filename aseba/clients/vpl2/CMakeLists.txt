find_package(Qt5 COMPONENTS Core Network Gui Qml Quick)
set(CMAKE_AUTOMOC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(ANDROID)
    add_library(thymiovpl2 SHARED
               src/app/main.cpp)
else()
    add_executable(thymiovpl2
               src/app/main.cpp)
endif()

target_sources(thymiovpl2 PRIVATE
    src/common/aseba.cpp
    src/common/thymio/ThymioManager.cpp
    src/common/thymio/NetworkDeviceProber.cpp
    src/common/thymio/DeviceQtConnection.cpp
    src/common/thymio/ThymioProviderInfo.cpp
    src/common/thymio/ThymioListModel.cpp
    src/common/aseba.h
    src/common/thymio/ThymioManager.h
    src/common/thymio/NetworkDeviceProber.h
    src/common/thymio/DeviceQtConnection.h
    src/common/thymio/ThymioProviderInfo.h
    src/common/thymio/ThymioListModel.h
    src/app/thymio-vpl2.cpp
    src/app/thymio-vpl2.h
    src/app/liveqmlreloadingengine.cpp
)

if(NOT ANDROID)
    target_sources(thymiovpl2 PRIVATE
        src/common/thymio/UsbSerialDeviceProber.cpp
        src/common/thymio/UsbSerialDeviceProber.h
    )
    find_package(Qt5 COMPONENTS SerialPort)
    target_link_libraries(thymiovpl2 PRIVATE Qt5::SerialPort)
endif()

if(ANDROID)
    target_sources(thymiovpl2 PRIVATE
        src/common/thymio/AndroidSerialDeviceProber.cpp
        src/common/thymio/AndroidSerialDeviceProber.h
        src/common/thymio/AndroidUsbSerialDevice.cpp
        src/common/thymio/AndroidUsbSerialDevice.h
    )
endif()

target_link_libraries(thymiovpl2 PUBLIC asebacommon asebacompiler Qt5::Core Qt5::Qml Qt5::Quick QtZeroConf libusb)

if(ANDROID)
    find_package(Qt5AndroidExtras)
    target_link_libraries(thymiovpl2 PUBLIC Qt5::AndroidExtras)
endif()

target_compile_definitions(thymiovpl2 PRIVATE -DQML_ROOT_FILE=\"qrc:/thymio-vpl2/main.qml\")
target_include_directories(thymiovpl2 PRIVATE src/common/)

file(GLOB TRANSLATIONS_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/translations/*.ts")
set_source_files_properties(${TRANSLATIONS_SOURCES} PROPERTIES OUTPUT_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/src/qml/thymio-vpl2/translations/")
qt5_add_translation(TRANSLATIONS ${TRANSLATIONS_SOURCES} )
qt5_add_resources(resfiles src/qml/thymio-vpl2.qrc)


if(ANDROID)
    set(ANDROID_NATIVE_API_LEVEL 20)
    add_qt_android_apk(
        thymiovpl2_apk thymiovpl2
        NAME "Thymio VPL"
        PACKAGE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/android"
        DEPENDS QtZeroConf libusb
        MANIFEST "${CMAKE_CURRENT_SOURCE_DIR}/android/AndroidManifest.xml"
        )
endif()
