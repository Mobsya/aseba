set(CMAKE_AUTOMOC on)


if(APPLE)
    SET(VPL_EXECUTABLE_NAME ThymioVPLClassic)
else()
    SET(VPL_EXECUTABLE_NAME thymiovplclassic)
endif()

add_executable(${VPL_EXECUTABLE_NAME} WIN32 MACOSX_BUNDLE
    thymiovpl.cpp
    ThymioVisualProgramming.cpp
    ResizingView.cpp
    Buttons.cpp
    Block.cpp
    EventBlocks.cpp
    StateBlocks.cpp
    ActionBlocks.cpp
    EventActionsSet.cpp
    Scene.cpp
    Compiler.cpp
    CompilerVisitors.cpp
    Style.cpp
    Utils.cpp
    LogSignalMapper.cpp
    ThymioVisualProgramming.h
    ResizingView.h
    Buttons.h
    Block.h
    EventBlocks.h
    ActionBlocks.h
    EventActionsSet.h
    Scene.h
    UsageLogger.h
    LogSignalMapper.h
    ThymioVPLApplication.h
    ThymioVPLApplication.cpp
)

target_link_libraries(${VPL_EXECUTABLE_NAME}
    asebaqtcommon
    qt-thymio-dm-client-lib
    asebaqtabout
    asebacommon
    Qt5::Help
    Qt5::Svg
    Qt5::Network
    Qt5::Xml
    Qt5::Widgets
)

install_qt_app(${VPL_EXECUTABLE_NAME})
codesign(${VPL_EXECUTABLE_NAME} )

if(APPLE)
    set(MACOSX_BUNDLE_BUNDLE_VERSION ${ASEBA_VERSION})
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING ${ASEBA_VERSION})
    set(MACOSX_BUNDLE_LONG_VERSION_STRING "${ASEBA_VERSION}-${GIT_REVISION}")
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "org.mobsya.ThymioVPLClassic")
    set(MACOSX_BUNDLE_ICON_FILE thymiovpl.icns)
    set(vpl_icon ${PROJECT_SOURCE_DIR}/menu/osx/thymiovpl.icns)
    set_source_files_properties(${vpl_icon} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    target_sources(${VPL_EXECUTABLE_NAME} PRIVATE ${vpl_icon})
endif()
