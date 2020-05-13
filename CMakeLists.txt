# emacs: -*- CMake -*-
# kate: replace-tabs off; tab-width 4; indent-width 4; tab-indents true; indent-mode normal
# vim: ts=4:sw=4:noexpandtab

if(APPLE AND NOT IOS)
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.12" CACHE STRING "Minimum osx version")
endif()

cmake_policy(SET CMP0071 NEW)
cmake_policy(SET CMP0020 NEW)
cmake_policy(SET CMP0054 NEW)
cmake_policy(SET CMP0003 NEW)


project(ThymioSuite)
set (CMAKE_PROJECT_VERSION 2.0)
set (CMAKE_PROJECT_VERSION_MAJOR 2)
set (CMAKE_PROJECT_VERSION_MINOR 0)
set(OpenGL_GL_PREFERENCE GLVND)


add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")



if(IOS)
    if(IOS_ARCHIVE_BUILD)
        set(CMAKE_XCODE_ATTRIBUTE_INSTALL_PATH "")
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin/libraries")
    endif()
endif()

# ensure a minimal version of CMake
cmake_minimum_required(VERSION 3.13)
include(InstallRequiredSystemLibraries)

# additional CMake modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/CMakeModules)

include(CMakeModules/ParseVersion.cmake)
include(CMakeModules/cpp_features.cmake)
include(CMakeModules/workarounds.cmake)
include(CMakeModules/aseba_conf.cmake)
include(CMakeModules/codesign.cmake)
include(CMakeModules/deployqt.cmake)
include(CMakeModules/CMakeQuery.cmake)

if(NOT ANDROID)
    if(NOT IOS)
    include(CMakeModules/boost.cmake)
    include(CMakeModules/enki.cmake)
    endif()
    find_package(Threads REQUIRED)
endif()

if(ANDROID)
    set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/android/qt-android-cmake/)
    include(android/qt-android-cmake/AddQtAndroidApk.cmake)
endif()

add_subdirectory(third_party/catch2 EXCLUDE_FROM_ALL)
add_subdirectory(third_party/libusb EXCLUDE_FROM_ALL)
add_subdirectory(third_party/qtzeroconf EXCLUDE_FROM_ALL)
add_subdirectory(third_party/spdlog EXCLUDE_FROM_ALL)
add_subdirectory(third_party/expected EXCLUDE_FROM_ALL)
add_subdirectory(third_party/fmt EXCLUDE_FROM_ALL)
add_subdirectory(third_party/qt-solutions EXCLUDE_FROM_ALL)
add_subdirectory(third_party/quazip EXCLUDE_FROM_ALL)

SET(BUILD_SHARED_LIBS_CACHED ${BUILD_SHARED_LIBS} CACHE BOOL "" FORCE)
SET(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/pugixml EXCLUDE_FROM_ALL)
SET(BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS_CACHED} CACHE BOOL "" FORCE)
SET(RANGE_V3_TESTS OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/range-v3 EXCLUDE_FROM_ALL)

add_library(mpark_variant INTERFACE)
target_include_directories(mpark_variant INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third_party/mpark.variant/include>
  $<INSTALL_INTERFACE:include>)

if (ANDROID OR IOS)
    set(FLATBUFFERS_BUILD_FLATC OFF CACHE BOOL "FLATBUFFERS_BUILD_FLATC" FORCE)
endif()

set(FLATBUFFERS_BUILD_TESTS OFF CACHE BOOL "FLATBUFFERS_BUILD_TESTS" FORCE)
add_subdirectory(third_party/flatbuffers EXCLUDE_FROM_ALL)

if (NOT TARGET flatc)
    add_executable(flatc IMPORTED GENERATED)
    if(NOT IOS)
        include( ExternalProject )
        list(APPEND CMAKE_ARGS "-DFLATBUFFERS_BUILD_FLATLIB=OFF")
        list(APPEND CMAKE_ARGS "-DCMAKE_BUILD_TYPE=Release")
        list(APPEND CMAKE_ARGS "-DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/flatc")

        ExternalProject_Add(flatc_build
        SOURCE_DIR ${PROJECT_SOURCE_DIR}/third_party/flatbuffers/
        CMAKE_ARGS "${CMAKE_ARGS}"
        BUILD_BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/flatc/bin/flatc
        PREFIX ${CMAKE_CURRENT_BINARY_DIR}/flatc
        )
        set_property(TARGET flatc PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/flatc/bin/flatc)
        add_dependencies(flatc flatc_build)
    else()
        set_property(TARGET flatc PROPERTY IMPORTED_LOCATION $ENV{FLATC})
    endif()
endif()


# testing and defines
enable_testing()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin")

cmake_policy(SET CMP0020 NEW)

if(CREATE_QT_CONF_FILE)
    #create an empty qt.conf file
    file(WRITE ${CMAKE_BINARY_DIR}/bin/qt.conf "")
    install(FILES ${CMAKE_BINARY_DIR}/bin/qt.conf DESTINATION bin)
endif()



# Qt
find_package(Qt5 OPTIONAL_COMPONENTS Core Gui Widgets OpenGL Network Concurrent Xml Svg Test LinguistTools Charts)

if (NOT ANDROID AND NOT IOS)
    find_package(Qt5Help)
endif()

if (UNIX AND NOT APPLE AND NOT ANDROID)
    find_package(Qt5X11Extras)
    find_package(Qt5DBus)
endif ()

# Zeroconf
if (NOT ANDROID AND NOT IOS)
    include(CMakeModules/zeroconf.cmake)
    add_subdirectory(third_party/aware)
endif()

# Aseba itself
set(LIB_INSTALL_DIR lib CACHE FILEPATH "Where to install libraries")
set(LIB_VERSION_MAJOR 3) # Must be bumped for incompatible ABI changes
set(LIB_VERSION_MINOR 0)
set(LIB_VERSION_PATCH 0)
set(LIB_VERSION_STRING ${LIB_VERSION_MAJOR}.${LIB_VERSION_MINOR}.${LIB_VERSION_PATCH})

find_package(Python3)

if(WIN32)
    include (MobsyaInstallRequiredSystemLibraries)
    add_subdirectory(third_party/bonjour)
    set(ASEBA_SOURCE_DIRECTORY "${PROJECT_SOURCE_DIR}")
    list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/windows)
    set(CPACK_MONOLITHIC_INSTALL 1)
    if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
        set(NSIS_DEFAULT_INSTALL_DIR "\$PROGRAMFILES64\\Thymio")
    else()
        set(NSIS_DEFAULT_INSTALL_DIR "\$PROGRAMFILES32\\Thymio")
    endif()

    configure_file(
        ${PROJECT_SOURCE_DIR}/windows/NSIS.definitions.nsh.in
        ${CMAKE_CURRENT_BINARY_DIR}/resources/NSIS.definitions.nsh
    )
endif()

set(CPACK_BUNDLE_NAME ThymioSuite)
set(CPACK_BUNDLE_PLIST ${PROJECT_SOURCE_DIR}/osx/Info.plist)
set(CPACK_BUNDLE_ICON ${PROJECT_SOURCE_DIR}/menu/osx/asebastudio.icns)


macro(install_symlink filepath sympath)
    install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${filepath} ${sympath})")
    install(CODE "message(\"-- Created symlink: ${sympath} -> ${filepath}\")")
endmacro(install_symlink)


SET(CPACK_PACKAGE_VERSION_MAJOR "${ASEBA_VERSION_MAJOR}")
SET(CPACK_PACKAGE_VERSION_MINOR "${ASEBA_VERSION_MINOR}")
SET(CPACK_PACKAGE_VERSION_PATCH "${ASEBA_VERSION_PATCH}")
include(CPack)

add_subdirectory(aseba)

if(NOT ANDROID AND NOT IOS)
	add_subdirectory(js)
endif()

if(NOT ANDROID AND NOT IOS)
    # Unit tests
    add_subdirectory(tests)
endif()


# User entries
if (UNIX)
    add_subdirectory(menu)
endif()

find_package(Python3)
if(Python3_EXECUTABLE)
    find_package(Qt5LinguistTools REQUIRED)
    configure_file(
        "${PROJECT_SOURCE_DIR}/maintainer/translations/langs.py"
        "${CMAKE_CURRENT_BINARY_DIR}/langs.py")
    add_custom_target("update-trads")
    add_custom_command(TARGET "update-trads" COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/langs.py)
endif()

if(FLAT_PACK_BUILD)
    SET(MOBSYA_WEBAPPS_ROOT_DIR "share/")
endif()

if(NOT IOS)
if(EXISTS ${PROJECT_SOURCE_DIR}/scratch/index.html)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/scratch/ DESTINATION "${MOBSYA_WEBAPPS_ROOT_DIR}scratch")
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/scratch/ DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${MOBSYA_WEBAPPS_ROOT_DIR}scratch")
endif()
if(EXISTS ${PROJECT_SOURCE_DIR}/thymio_blockly/index.html)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/thymio_blockly/ DESTINATION "${MOBSYA_WEBAPPS_ROOT_DIR}thymio_blockly")
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/thymio_blockly/ DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${MOBSYA_WEBAPPS_ROOT_DIR}thymio_blockly")
endif()
if(EXISTS ${PROJECT_SOURCE_DIR}/vpl3-thymio-suite/index.html)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/vpl3-thymio-suite/ DESTINATION "${MOBSYA_WEBAPPS_ROOT_DIR}vpl3-thymio-suite")
    file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/vpl3-thymio-suite/ DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${MOBSYA_WEBAPPS_ROOT_DIR}vpl3-thymio-suite")
endif()
endif() #NOT IOS
