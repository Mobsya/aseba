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
   # EventViewer.h
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
    # EventViewer.cpp
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


#set(DOC_NAME aseba-doc)
## generates the documentation
#if (Qt5Help_FOUND AND TARGET Qt5::qcollectiongenerator)
#    message(STATUS "qcollectiongenerator executable found")
#    get_property(QT_QCOLLECTIONGENERATOR_LOCATION TARGET Qt5::qcollectiongenerator PROPERTY IMPORTED_LOCATION)
#    get_filename_component(QT_QCOLLECTIONGENERATOR_DIRECTORY ${QT_QCOLLECTIONGENERATOR_LOCATION} DIRECTORY)
#    get_filename_component(QT_QCOLLECTIONGENERATOR_EXT ${QT_QCOLLECTIONGENERATOR_LOCATION} EXT)
#    set(QT_QHELPGENERATOR_EXECUTABLE "${QT_QCOLLECTIONGENERATOR_DIRECTORY}/qhelpgenerator${QT_QCOLLECTIONGENERATOR_EXT}")
#    set(QT_QCOLLECTIONGENERATOR_EXECUTABLE "${QT_QCOLLECTIONGENERATOR_DIRECTORY}/qcollectiongenerator${QT_QCOLLECTIONGENERATOR_EXT}")
#    add_custom_command(
#        OUTPUT ${DOC_NAME}.qch
#        COMMAND ${QT_QHELPGENERATOR_EXECUTABLE} ARGS ${CMAKE_CURRENT_SOURCE_DIR}/${DOC_NAME}.qhp -o ${CMAKE_CURRENT_BINARY_DIR}/${DOC_NAME}.qch
#        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${DOC_NAME}.qhp)
#    add_custom_command(
#        OUTPUT ${DOC_NAME}.qhc
#        COMMAND ${QT_QCOLLECTIONGENERATOR_EXECUTABLE} ARGS ${DOC_NAME}.qhcp -o ${DOC_NAME}.qhc
#        DEPENDS ${DOC_NAME}.qhcp ${DOC_NAME}.qch)
#else ()
#    message(WARNING "Qt help support missing, studio online help disabled (hint: on Ubuntu, install package qttools5-dev-tools)")
#endif ()
#if (QT_QCOLLECTIONGENERATOR_EXECUTABLE)
#    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/asebastudiohelp.qrc ${CMAKE_CURRENT_SOURCE_DIR}/aseba-doc.qhcp DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
#    qt5_add_resources(resfiles ${CMAKE_CURRENT_BINARY_DIR}/asebastudiohelp.qrc)
#endif ()