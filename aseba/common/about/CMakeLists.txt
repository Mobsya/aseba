include_directories(${CMAKE_BINARY_DIR})
add_definitions(-DHAVE_ENKI)

qt5_add_resources(resfiles asebaqtabout.qrc)

file(GLOB TRANSLATIONS_SOURCES "qtabout*.ts")
set_source_files_properties(${TRANSLATIONS_SOURCES} PROPERTIES OUTPUT_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}")
qt5_add_translation(TRANSLATIONS ${TRANSLATIONS_SOURCES} )

qt5_wrap_cpp(qt_about_MOCS AboutDialog.h)
add_library(asebaqtabout STATIC AboutDialog.cpp ${qt_about_MOCS} ${resfiles})
target_link_libraries(asebaqtabout asebacommon Qt5::Core Qt5::Widgets Qt5::Svg)

if (HAS_DYN_VERSION)
    add_dependencies(asebaqtabout versionheader)
endif (HAS_DYN_VERSION)
set_target_properties(asebaqtabout PROPERTIES VERSION ${LIB_VERSION_STRING})
