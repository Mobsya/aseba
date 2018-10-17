# kate: replace-tabs off; tab-width 4; indent-width 4; tab-indents true; indent-mode normal
# vim: ts=4:sw=4:noexpandtab
set(DOC_NAME aseba-doc)

set(CMAKE_AUTOMOC on)

# add a console for debug messages under Windows
if (WIN32)
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "-Wl,-subsystem,console ${CMAKE_EXE_LINKER_FLAGS}")
endif ()

# generates the documentation
if (Qt5Help_FOUND AND TARGET Qt5::qcollectiongenerator)
    message(STATUS "qcollectiongenerator executable found")
    get_property(QT_QCOLLECTIONGENERATOR_LOCATION TARGET Qt5::qcollectiongenerator PROPERTY IMPORTED_LOCATION)
    get_filename_component(QT_QCOLLECTIONGENERATOR_DIRECTORY ${QT_QCOLLECTIONGENERATOR_LOCATION} DIRECTORY)
    get_filename_component(QT_QCOLLECTIONGENERATOR_EXT ${QT_QCOLLECTIONGENERATOR_LOCATION} EXT)
    set(QT_QHELPGENERATOR_EXECUTABLE "${QT_QCOLLECTIONGENERATOR_DIRECTORY}/qhelpgenerator${QT_QCOLLECTIONGENERATOR_EXT}")
    set(QT_QCOLLECTIONGENERATOR_EXECUTABLE "${QT_QCOLLECTIONGENERATOR_DIRECTORY}/qcollectiongenerator${QT_QCOLLECTIONGENERATOR_EXT}")
    add_custom_command(
        OUTPUT ${DOC_NAME}.qch
        COMMAND ${QT_QHELPGENERATOR_EXECUTABLE} ARGS ${CMAKE_CURRENT_SOURCE_DIR}/${DOC_NAME}.qhp -o ${CMAKE_CURRENT_BINARY_DIR}/${DOC_NAME}.qch
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${DOC_NAME}.qhp)
    add_custom_command(
        OUTPUT ${DOC_NAME}.qhc
        COMMAND ${QT_QCOLLECTIONGENERATOR_EXECUTABLE} ARGS ${DOC_NAME}.qhcp -o ${DOC_NAME}.qhc
        DEPENDS ${DOC_NAME}.qhcp ${DOC_NAME}.qch)
else ()
    message(WARNING "Qt help support missing, studio online help disabled (hint: on Ubuntu, install package qttools5-dev-tools)")
endif ()

# code common to the use of Aseba under Qt
set(qt_common_SRCS
    Target.cpp
    TargetModels.cpp
    CustomWidgets.cpp
    AeslEditor.cpp
    translations/CompilerTranslator.cpp
)
qt5_wrap_cpp(qt_common_MOCS
    Target.h
    TargetModels.h
    AeslEditor.h
    translations/CompilerTranslator.h
)
add_library(asebaqtcommon STATIC ${qt_common_SRCS} ${qt_common_MOCS})
target_link_libraries(asebaqtcommon aseba_conf)
target_link_libraries(asebaqtcommon Qt5::Network Qt5::Xml Qt5::Widgets)

codesign(asebaqtcommon)

# plugins
#add_subdirectory(plugins)

# resources
file(GLOB TRANSLATIONS_SOURCES "translations/asebastudio*.ts"  "translations/compiler*.ts")
set_source_files_properties(${TRANSLATIONS_SOURCES} PROPERTIES OUTPUT_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/translations")
qt5_add_translation(TRANSLATIONS ${TRANSLATIONS_SOURCES} )
qt5_add_resources(resfiles asebastudio.qrc)

include_directories (${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR})

add_executable(asebastudio WIN32
    StudioAeslEditor.h
    MainWindow.h
    NamedValuesVectorModel.h
    ClickableLabel.h
    CustomDelegate.h
    FindDialog.h
   # EventViewer.h
    HelpViewer.h
    ConfigDialog.h
    ModelAggregator.h
    StudioAeslEditor.cpp
    MainWindow.cpp
    NamedValuesVectorModel.cpp
    ClickableLabel.cpp
    CustomDelegate.cpp
    NodeTab.h
    NodeTab.cpp
    NodeTabsManager.cpp
    NodeTabsManager.h
    main.cpp
    FindDialog.cpp
    # EventViewer.cpp
    HelpViewer.cpp
    ConfigDialog.cpp
    ModelAggregator.cpp
    ${resfiles}
)

if (QT_QCOLLECTIONGENERATOR_EXECUTABLE)
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/asebastudiohelp.qrc ${CMAKE_CURRENT_SOURCE_DIR}/aseba-doc.qhcp DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    qt5_add_resources(resfiles ${CMAKE_CURRENT_BINARY_DIR}/asebastudiohelp.qrc)
endif ()

if (HAS_DYN_VERSION)
    add_dependencies(asebastudio versionheader)
endif ()

target_link_libraries(asebastudio qt-thymio-dm-client-lib asebaqtcommon asebaqtabout asebacommon ${ADDITIONAL_LIBRARIES} Qt5::Help Qt5::Svg Qt5::Concurrent Qt5::Network Qt5::Xml Qt5::Widgets)
#asebaqtplugins

install_qt_app(asebastudio)
codesign(asebastudio)

# Thymio VPL standalone

#if (ANDROID)
#    add_library(thymiovpl SHARED ${thymiovpl_SRCS} ${resfiles})
#else ()
#    add_executable(thymiovpl WIN32 ${thymiovpl_SRCS} ${resfiles})
#endif ()
#if (HAS_DYN_VERSION)
#    add_dependencies(thymiovpl versionheader)
#endif ()

#target_include_directories(thymiovpl PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/plugins/ThymioVPL)
#qt5_wrap_cpp(vpl_MOCS
#    ThymioVPLStandalone.h
#)
##target_sources(thymiovpl PUBLIC ThymioVPLStandalone.cpp ${vpl_MOCS} thymiovpl.cpp)
#target_link_libraries(thymiovpl asebaqtplugins asebaqtcommon asebacompiler asebaqtabout asebacommon ${ADDITIONAL_LIBRARIES} Qt5::Svg Qt5::Network Qt5::Xml Qt5::Widgets)
#codesign(thymiovpl)

#renderblocks
#add_executable(rendervplblocks ${CMAKE_CURRENT_SOURCE_DIR}/rendervplblocks.cpp ${resfiles})
#target_link_libraries(rendervplblocks asebaqtplugins asebaqtcommon asebaqtabout asebacompiler asebacommon ${ADDITIONAL_LIBRARIES} Qt5::Xml Qt5::Svg Qt5::Widgets)
#install_qt_app(rendervplblocks)
#codesign(rendervplblocks)
