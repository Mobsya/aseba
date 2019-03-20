set(CMAKE_AUTOMOC on)

file(GLOB TRANSLATIONS_SOURCES "translations/asebastudio*.ts"  "translations/compiler*.ts")
set_source_files_properties(${TRANSLATIONS_SOURCES} PROPERTIES OUTPUT_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/translations")
qt5_add_translation(TRANSLATIONS ${TRANSLATIONS_SOURCES} )
qt5_add_resources(resfiles asebastudio.qrc)

add_library(asebaqtcommon OBJECT
    MobsyaApplication.cpp
    MobsyaApplication.h
    common/CustomWidgets.cpp
    common/CustomWidgets.h
    common/AeslEditor.cpp
    common/AeslEditor.h
    translations/CompilerTranslator.cpp
    translations/CompilerTranslator.h
    ${resfiles}
)
target_link_libraries(asebaqtcommon PUBLIC asebacompiler Qt5::Xml Qt5::Widgets)
target_include_directories(asebaqtcommon PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})