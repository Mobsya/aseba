set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
cmake_policy(SET CMP0071 NEW)
set(CMAKE_CXX_STANDARD 17)

find_package(Qt5QuickCompiler)
if((NOT FLAT_PACK_BUILD)  AND Qt5QuickCompiler_FOUND)
    qtquick_compiler_add_resources(QT_RESOURCES_BIN qml.qrc)
endif()
qt5_add_resources(QT_RESOURCES qml.qrc)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/launcher.rc.in
  ${CMAKE_CURRENT_BINARY_DIR}/launcher.rc)

file(GLOB TRANSLATIONS_SOURCES "translations/*.ts")
set_source_files_properties(${TRANSLATIONS_SOURCES} PROPERTIES OUTPUT_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/translations")
qt5_add_translation(TRANSLATIONS ${TRANSLATIONS_SOURCES} )
qt5_add_resources(QT_RESOURCES_TRANSLATIONS translations.qrc)

if(ANDROID)
    add_library(thymio-launcher SHARED)
else()
    add_executable(thymio-launcher WIN32 MACOSX_BUNDLE)
endif()
target_sources(thymio-launcher PRIVATE
    ${QT_RESOURCES}
    ${QT_RESOURCES_PRECOMPILED}
    ${QT_RESOURCES_TRANSLATIONS}
    main.cpp
    launcher.h
    launcher.cpp
    launcherwindow.h
    launcherwindow.cpp
    tdmsupervisor.h
    tdmsupervisor.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/launcher.rc
)

if(APPLE)
    target_sources(thymio-launcher PRIVATE launcher.mm)
endif()

target_link_libraries(thymio-launcher
    asebacommon
    Qt5::Widgets
    Qt5::Qml
    Qt5::Quick
    Qt5::QuickWidgets
    Qt5::Svg
    ${Qt5Core_QTMAIN_LIBRARIES}
    qt-thymio-dm-client-lib
    qt-thymio-dm-client-lib-qml
    qtsingleapplication
)

if(APPLE OR ANDROID)
    target_link_libraries(thymio-launcher Qt5::WebView)
else()
    target_link_libraries(thymio-launcher Qt5::WebEngine)
    target_compile_definitions(thymio-launcher PRIVATE -DMOBSYA_USE_WEBENGINE)
endif()

if(WIN32)
    query_qmake(QWEBENGINE_PATH QT_INSTALL_LIBEXECS)
    if(NOT CMAKE_BUILD_TYPE MATCHES Release)
            configure_file("${QWEBENGINE_PATH}/QtWebEngineProcessd.exe" ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG} COPYONLY)
    endif()
    message(${CMAKE_RUNTIME_OUTPUT_DIRECTORY} "${QWEBENGINE_PATH}/QtWebEngineProcess.exe")
    configure_file("${QWEBENGINE_PATH}/QtWebEngineProcess.exe" ${CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE} COPYONLY)
endif()

if(NOT ANDROID)
    codesign(thymio-launcher)
    install_qt_app(thymio-launcher)
endif()

if(NOT CMAKE_BUILD_TYPE MATCHES Release)
    target_compile_definitions(thymio-launcher PRIVATE -DQT_QML_DEBUG)
endif()

if(APPLE)
    find_library(FOUNDATION_FRAMEWORK Foundation)
    find_library(AppKit_FRAMEWORK AppKit)
    target_link_libraries(thymio-launcher ${FOUNDATION_FRAMEWORK} ${AppKit_FRAMEWORK})



    set(MACOSX_BUNDLE_INFO_PLIST ${PROJECT_SOURCE_DIR}/osx/Launcher.plist)
    set(MACOSX_BUNDLE_BUNDLE_NAME "Thymio Suite")
    set(MACOSX_BUNDLE_BUNDLE_VERSION ${ASEBA_VERSION})
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING ${ASEBA_VERSION})
    set(MACOSX_BUNDLE_LONG_VERSION_STRING "${ASEBA_VERSION}-${GIT_REVISION}")
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "org.mobsya.ThymioLauncher")
    set(MACOSX_BUNDLE_ICON_FILE launcher.icns)
    set(launcher_icon ${PROJECT_SOURCE_DIR}/menu/osx/launcher.icns)
    set_source_files_properties(${launcher_icon} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    target_sources(thymio-launcher PRIVATE ${launcher_icon})
endif()


if(ANDROID)
    add_qt_android_apk(thymio-launcher-apk thymio-launcher
        PACKAGE_NAME org.mobsya.ThymioSuite
        QML_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/qml
    )
endif()

