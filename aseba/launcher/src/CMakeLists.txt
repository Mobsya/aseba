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

if(IOS)

  target_sources(thymio-launcher PUBLIC "${PROJECT_SOURCE_DIR}/scratch")
  SET_SOURCE_FILES_PROPERTIES("${PROJECT_SOURCE_DIR}/scratch" PROPERTIES MACOSX_PACKAGE_LOCATION "webapps")

  target_sources(thymio-launcher PUBLIC "${PROJECT_SOURCE_DIR}/thymio_blockly")
  SET_SOURCE_FILES_PROPERTIES("${PROJECT_SOURCE_DIR}/thymio_blockly" PROPERTIES MACOSX_PACKAGE_LOCATION "webapps")
endif()

if(APPLE)
    target_sources(thymio-launcher PRIVATE launcher.mm)
endif()

if(IOS)
    #use the addQtIos cmake to manage links
    INCLUDE("${PROJECT_SOURCE_DIR}/ios/AddQtIosApp.cmake")
    INCLUDE("${PROJECT_SOURCE_DIR}/ios/QtStaticCMake.cmake")
endif()

if(IOS)
    #use the addQtIos cmake to manage links
    qt5_use_modules( thymio-launcher Core  QuickControls2  QuickWidgets QuickTemplates2 Svg Qml Quick)
    #target_link_libraries(thymio-launcher )
    qt_generate_qml_plugin_import(thymio-launcher QML_SRC ${CMAKE_CURRENT_SOURCE_DIR}/qml VERBOSE)
    qt_generate_plugin_import(thymio-launcher VERBOSE)
endif()





if(IOS)
    #Getting the timestamp to use it as a version. (Apple Store require numbers, and check that the next version is higher than the current one)
    string(TIMESTAMP CUR_TIME %s)

    set (SIGNING_ID "P97H86YL8K")
    #Code signing should not be necessary since it's set to auto.
    #if(IOS AND IOS_ARCHIVE_BUILD)
    #    set (CODE_SIGN_IDENTITY "iPhone Distribution")
    #else()
    #    set (CODE_SIGN_IDENTITY "iPhone Developer")
    #endif()

    #create app reference
    add_qt_ios_app(thymio-launcher
        NAME "Thymio Suite"
        BUNDLE_IDENTIFIER "org.mobsya.thymio.launcher.ios"
        VERSION ${CUR_TIME}
        SHORT_VERSION ${ASEBA_VERSION}
        LONG_VERSION "${ASEBA_VERSION}-${GIT_REVISION}"
        CUSTOM_PLIST "${PROJECT_SOURCE_DIR}/ios/MacOSXBundleInfo.plist.in"
        TEAM_ID ${SIGNING_ID}
        COPYRIGHT "Mobysa"
        QML_DIR "${QT_WINDOWS_QT_ROOT}/qml"
        ASSET_DIR "${PROJECT_SOURCE_DIR}/ios/bundle/Assets.xcassets"
        MAIN_STORYBOARD "${PROJECT_SOURCE_DIR}/ios/bundle/Main.storyboard"
                    LAUNCHSCREEN_STORYBOARD "${PROJECT_SOURCE_DIR}/ios/bundle/LaunchScreen.storyboard"
                    CATALOG_APPICON "AppIcon"
                    CATALOG_LAUNCHIMAGE "LaunchImage"
        ORIENTATION_PORTRAIT
        ORIENTATION_PORTRAIT_UPDOWN
        ORIENTATION_LANDSCAPE_LEFT
        ORIENTATION_LANDSCAPE_RIGHT
        SUPPORT_IPAD
        REQUIRES_FULL_SCREEN
        VERBOSE
     )

 qt_ios_set_xcode_property(thymio-launcher IPHONEOS_DEPLOYMENT_TARGET "${IOS_DEPLOYMENT_TARGET}" "Debug")
 qt_ios_set_xcode_property(thymio-launcher IPHONEOS_DEPLOYMENT_TARGET "${IOS_DEPLOYMENT_TARGET}" "MinSizeRel")
 qt_ios_set_xcode_property(thymio-launcher IPHONEOS_DEPLOYMENT_TARGET  "${IOS_DEPLOYMENT_TARGET}" "Release")
endif()

target_link_libraries(thymio-launcher
    Qt5::Widgets
    Qt5::Qml
    Qt5::Quick
    Qt5::QuickWidgets
    Qt5::Svg
    ${Qt5Core_QTMAIN_LIBRARIES}
)

target_link_libraries(thymio-launcher
    asebacommon
    qt-thymio-dm-client-lib
    qt-thymio-dm-client-lib-qml
    qtsingleapplication
)

if(IOS AND IOS_ARCHIVE_BUILD)

    set_target_properties(thymio-launcher PROPERTIES
    XCODE_ATTRIBUTE_INSTALL_PATH "/Applications"
    XCODE_ATTRIBUTE_SKIP_INSTALL "No"
    )
endif()

#file(GLOB_RECURSE QT_ADDITIONAL_PLUGINS ${CMAKE_PREFIX_PATH}/qml/*.a)

#target_link_libraries(thymio-launcher
#      ${QT_ADDITIONAL_PLUGINS}
#    )

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

if(NOT ANDROID AND NOT IOS)
    codesign(thymio-launcher)
    install_qt_app(thymio-launcher)
endif()



if(NOT CMAKE_BUILD_TYPE MATCHES Release)
    target_compile_definitions(thymio-launcher PRIVATE -DQT_QML_DEBUG)
endif()

if(APPLE)
    find_library(FOUNDATION_FRAMEWORK Foundation)
    if(IOS)
          find_library(UIKit_FRAMEWORK UIKit)
    else()
          find_library(AppKit_FRAMEWORK AppKit)
    endif()

    if(NOT IOS)
        #IOS part should already be handled via the dedicated cmake
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
    endif() #NOT IOS
endif()



if(ANDROID)
	FetchContent_Declare(
		QtAndroidCMake
		GIT_REPOSITORY https://github.com/Mobsya/qt-android-cmake
		GIT_TAG        master
	)
	FetchContent_MakeAvailable(QtAndroidCMake)
    add_qt_android_apk(thymio-launcher-apk thymio-launcher
        PACKAGE_NAME org.mobsya.ThymioSuite
        QML_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/qml
    )
endif()






