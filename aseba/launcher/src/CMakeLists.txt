set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
cmake_policy(SET CMP0071 NEW)
set(CMAKE_CXX_STANDARD 17)

find_package(Qt5QuickCompiler)
if(Qt5QuickCompiler_FOUND)
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
  SET_SOURCE_FILES_PROPERTIES("${PROJECT_SOURCE_DIR}/scratch" PROPERTIES MACOSX_PACKAGE_LOCATION "Share")


#add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/thymio-launcher.app
#    COMMAND  ${CMAKE_COMMAND}  -E copy_directory ${CMAKE_CURRENT_BINARY_DIR}/scratch ${CMAKE_CURRENT_BINARY_DIR}/thymio-launcher.app )
##add_custom_target(${CMAKE_CURRENT_BINARY_DIR}/${LOCATION_FOR_THE_APP} DEPENDS

endif()

if(APPLE)
    target_sources(thymio-launcher PRIVATE launcher.mm)
endif()

if(IOS)
    #use the addQtIos cmake to manage links
    INCLUDE(../../../ios/AddQtIosApp.cmake)
    INCLUDE(../../../ios/QtStaticCMake.cmake)
endif()
if(IOS)
    #use the addQtIos cmake to manage links
    qt5_use_modules( thymio-launcher Core  QuickControls2  QuickWidgets QuickTemplates2 Svg Qml Quick)
    target_link_libraries(thymio-launcher )


    qt_generate_qml_plugin_import(thymio-launcher QML_SRC ${CMAKE_CURRENT_SOURCE_DIR}/qml VERBOSE)
    qt_generate_plugin_import(thymio-launcher VERBOSE)
endif()
if(IOS)
    #create app reference
    add_qt_ios_app(thymio-launcher
        NAME "thymio-launcher"
        BUNDLE_IDENTIFIER "ch.icare.thymio.launcher"
        VERSION "1.2.3"
        SHORT_VERSION "1.2.3"
        LONG_VERSION "1.2.3.456"
      #  CUSTOM_PLIST "path/to/MacOSXBundleInfo.plist.in"
        CODE_SIGN_IDENTITY "iPhone Developer"
        TEAM_ID "K62V5ND5Y9"
        COPYRIGHT "My Cool Copyright"
        QML_DIR "${QT_WINDOWS_QT_ROOT}/qml"
        ASSET_DIR "${PROJECT_SOURCE_DIR}/ios/bundle/Assets.xcassets"
        MAIN_STORYBOARD "${PROJECT_SOURCE_DIR}/ios/bundle/Main.storyboard"
                    LAUNCHSCREEN_STORYBOARD "${PROJECT_SOURCE_DIR}/ios/bundle/LaunchScreen.storyboard"
                    CATALOG_APPICON "AppIcon"
    #                CATALOG_LAUNCHIMAGE "LaunchImage"
        ORIENTATION_PORTRAIT
        ORIENTATION_PORTRAIT_UPSIDEDOWN
        ORIENTATION_LANDSCAPELEFT
        ORIENTATION_LANDSCAPERIGHT
        SUPPORT_IPAD
        SUPPORT_IPHONE
        REQUIRES_FULL_SCREEN
        HIDDEN_STATUS_BAR
        VERBOSE
     )
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
    qtsingleapplication)

#TODO make it dynamic. Also I've put everything ...
target_link_libraries(thymio-launcher
    /Users/abbet/projects/Qt/5.13.0/ios/qml/QtWebView/libdeclarative_webview.a
    /Users/abbet/projects/Qt/5.13.0/ios/qml/QtWebView/libdeclarative_webview_debug.a
    /Users/abbet/projects/Qt/5.13.0/ios/qml/Qt/labs/settings/libqmlsettingsplugin.a
    /Users/abbet/projects/Qt/5.13.0/ios/qml/Qt/labs/settings/libqmlsettingsplugin_debug.a
    /Users/abbet/projects/Qt/5.13.0/ios/qml/Qt/labs/folderlistmodel/libqmlfolderlistmodelplugin.a
    /Users/abbet/projects/Qt/5.13.0/ios/qml/Qt/labs/folderlistmodel/libqmlfolderlistmodelplugin_debug.a
    /Users/abbet/projects/Qt/5.13.0/ios/qml/QtQuick.2/libqtquick2plugin.a
    /Users/abbet/projects/Qt/5.13.0/ios/qml/QtQuick.2/libqtquick2plugin_debug.a
    /Users/abbet/projects/Qt/5.13.0/ios/qml/QtGraphicalEffects/libqtgraphicaleffectsplugin_debug.a
    /Users/abbet/projects/Qt/5.13.0/ios/qml/QtGraphicalEffects/libqtgraphicaleffectsplugin.a
    /Users/abbet/projects/Qt/5.13.0/ios/qml/QtGraphicalEffects/private/libqtgraphicaleffectsprivate.a
    /Users/abbet/projects/Qt/5.13.0/ios/qml/QtGraphicalEffects/private/libqtgraphicaleffectsprivate_debug.a
    /Users/abbet/projects/Qt/5.13.0/ios/qml/QtQuick/PrivateWidgets/libwidgetsplugin.a
    /Users/abbet/projects/Qt/5.13.0/ios/qml/QtQuick/PrivateWidgets/libwidgetsplugin_debug.a
    /Users/abbet/projects/Qt/5.13.0/ios/qml/QtQuick/Templates.2/libqtquicktemplates2plugin.a
    /Users/abbet/projects/Qt/5.13.0/ios/qml/QtQuick/Templates.2/libqtquicktemplates2plugin_debug.a
    /Users/abbet/projects/Qt/5.13.0/ios/qml/QtQuick/Controls/libqtquickcontrolsplugin_debug.a
    /Users/abbet/projects/Qt/5.13.0/ios/qml/QtQuick/Controls/Styles/Flat/libqtquickextrasflatplugin.a
    /Users/abbet/projects/Qt/5.13.0/ios/qml/QtQuick/Controls/Styles/Flat/libqtquickextrasflatplugin_debug.a
    /Users/abbet/projects/Qt/5.13.0/ios/qml/QtQuick/Controls/libqtquickcontrolsplugin.a
    /Users/abbet/projects/Qt/5.13.0/ios/qml/QtQuick/Controls.2/Fusion/libqtquickcontrols2fusionstyleplugin_debug.a
    /Users/abbet/projects/Qt/5.13.0/ios/qml/QtQuick/Controls.2/Fusion/libqtquickcontrols2fusionstyleplugin.a
    /Users/abbet/projects/Qt/5.13.0/ios/qml/QtQuick/Controls.2/Universal/libqtquickcontrols2universalstyleplugin_debug.a
    /Users/abbet/projects/Qt/5.13.0/ios/qml/QtQuick/Controls.2/Universal/libqtquickcontrols2universalstyleplugin.a
    /Users/abbet/projects/Qt/5.13.0/ios/qml/QtQuick/Controls.2/libqtquickcontrols2plugin_debug.a
    /Users/abbet/projects/Qt/5.13.0/ios/qml/QtQuick/Controls.2/libqtquickcontrols2plugin.a
    /Users/abbet/projects/Qt/5.13.0/ios/qml/QtQuick/Controls.2/Material/libqtquickcontrols2materialstyleplugin.a
    /Users/abbet/projects/Qt/5.13.0/ios/qml/QtQuick/Controls.2/Material/libqtquickcontrols2materialstyleplugin_debug.a
    /Users/abbet/projects/Qt/5.13.0/ios/qml/QtQuick/Controls.2/Imagine/libqtquickcontrols2imaginestyleplugin_debug.a
    /Users/abbet/projects/Qt/5.13.0/ios/qml/QtQuick/Controls.2/Imagine/libqtquickcontrols2imaginestyleplugin.a
    /Users/abbet/projects/Qt/5.13.0/ios/qml/QtQuick/Layouts/libqquicklayoutsplugin_debug.a
    /Users/abbet/projects/Qt/5.13.0/ios/qml/QtQuick/Layouts/libqquicklayoutsplugin.a
    /Users/abbet/projects/Qt/5.13.0/ios/qml/QtQuick/Dialogs/libdialogplugin_debug.a
    /Users/abbet/projects/Qt/5.13.0/ios/qml/QtQuick/Dialogs/Private/libdialogsprivateplugin.a
    /Users/abbet/projects/Qt/5.13.0/ios/qml/QtQuick/Dialogs/Private/libdialogsprivateplugin_debug.a
    /Users/abbet/projects/Qt/5.13.0/ios/qml/QtQuick/Dialogs/libdialogplugin.a
    /Users/abbet/projects/Qt/5.13.0/ios/qml/QtQuick/Window.2/libwindowplugin_debug.a
    /Users/abbet/projects/Qt/5.13.0/ios/qml/QtQuick/Window.2/libwindowplugin.a
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
    add_qt_android_apk(thymio-launcher-apk thymio-launcher
        PACKAGE_NAME org.mobsya.ThymioSuite
        QML_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/qml
    )
endif()






