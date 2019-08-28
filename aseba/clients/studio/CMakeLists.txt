set(CMAKE_AUTOMOC on)

if(APPLE)
    SET(STUDIO_EXECUTABLE_NAME AsebaStudio)
else()
    SET(STUDIO_EXECUTABLE_NAME asebastudio)
endif()


add_executable(${STUDIO_EXECUTABLE_NAME} WIN32 MACOSX_BUNDLE
    StudioAeslEditor.h
    MainWindow.h
    NamedValuesVectorModel.h
    ClickableLabel.h
    CustomDelegate.h
    ConstantsWidget.h
    ConstantsWidget.cpp
    EventsWidget.h
    EventsWidget.cpp
    FindDialog.h
    ConfigDialog.h
    ModelAggregator.h
    StudioAeslEditor.cpp
    MainWindow.cpp
    NamedValuesVectorModel.cpp
    ClickableLabel.cpp
    CustomDelegate.cpp
    NodeTab.h
    NodeTab.cpp
    PlotTab.h
    PlotTab.cpp
    NodeTabsManager.cpp
    NodeTabsManager.h
    LockButton.h
    LockButton.cpp
    TargetFunctionsModel.cpp
    TargetFunctionsModel.h
    main.cpp
    FindDialog.cpp
    ConfigDialog.cpp
    ModelAggregator.cpp
    VariablesModel.h
    VariablesModel.cpp
    NewNamedValueDialog.h
    NewNamedValueDialog.cpp
)

target_link_libraries(${STUDIO_EXECUTABLE_NAME}
    asebaqtcommon
    qt-thymio-dm-client-lib
    asebaqtabout
    asebacommon
    Qt5::Svg
    Qt5::Network
    Qt5::Xml
    Qt5::Widgets
    Qt5::Charts
    ${Qt5Core_QTMAIN_LIBRARIES}
    range-v3
)

install_qt_app(${STUDIO_EXECUTABLE_NAME})
codesign(${STUDIO_EXECUTABLE_NAME})

if(APPLE)
    set(MACOSX_BUNDLE_BUNDLE_VERSION ${ASEBA_VERSION})
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING ${ASEBA_VERSION})
    set(MACOSX_BUNDLE_LONG_VERSION_STRING "${ASEBA_VERSION}-${GIT_REVISION}")
    set(MACOSX_BUNDLE_GUI_IDENTIFIER "org.mobsya.AsebaStudio")
    set(MACOSX_BUNDLE_ICON_FILE asebastudio.icns)
    set(studio_icon ${PROJECT_SOURCE_DIR}/menu/osx/asebastudio.icns)
    set_source_files_properties(${studio_icon} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    target_sources(${STUDIO_EXECUTABLE_NAME} PRIVATE ${studio_icon})
endif()