# emacs: -*- CMake -*-
# kate: replace-tabs off; tab-width 4; indent-width 4; tab-indents true; indent-mode normal
# vim: ts=4:sw=4:noexpandtab

# project name
project(aseba)

add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

cmake_policy(SET CMP0071 NEW)


# ensure a minimal version of CMake
cmake_minimum_required(VERSION 3.5)
include(FeatureSummary)
include(InstallRequiredSystemLibraries)

# additional CMake modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/CMakeModules)

include(CMakeModules/ParseVersion.cmake)
include(CMakeModules/cpp_features.cmake)
include(CMakeModules/workarounds.cmake)
include(CMakeModules/aseba_conf.cmake)
include(CMakeModules/codesign.cmake)
include(CMakeModules/dashel.cmake)
include(CMakeModules/deployqt.cmake)
include(CMakeModules/CMakeQuery.cmake)

if(NOT ANDROID)
    include(CMakeModules/boost.cmake)
    include(CMakeModules/enki.cmake)
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

SET(BUILD_SHARED_LIBS_CACHED ${BUILD_SHARED_LIBS} CACHE BOOL "" FORCE)
SET(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
add_subdirectory(third_party/pugixml EXCLUDE_FROM_ALL)
SET(BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS_CACHED} CACHE BOOL "" FORCE)
add_subdirectory(third_party/range-v3 EXCLUDE_FROM_ALL)

add_library(mpark_variant INTERFACE)
target_include_directories(mpark_variant INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/third_party/mpark.variant/include>
  $<INSTALL_INTERFACE:include>)

if (ANDROID)
    set(FLATBUFFERS_BUILD_FLATC OFF CACHE BOOL "FLATBUFFERS_BUILD_FLATC" FORCE)
endif()
add_subdirectory(third_party/flatbuffers)

if (NOT TARGET flatc)
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
    add_executable(flatc IMPORTED GENERATED)
    set_property(TARGET flatc PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_BINARY_DIR}/flatc/bin/flatc)
    add_dependencies(flatc flatc_build)
endif()

# testing and defines
enable_testing()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin")

if(CREATE_QT_CONF_FILE)
    #create an empty qt.conf file
    file(WRITE ${CMAKE_BINARY_DIR}/bin/qt.conf "")
    install(FILES ${CMAKE_BINARY_DIR}/bin/qt.conf DESTINATION bin)
endif()



# Qt and Qwt
find_package(Qt5 OPTIONAL_COMPONENTS Core Gui Widgets OpenGL Network Concurrent Xml Svg Test LinguistTools Charts)

if (NOT ANDROID)
    find_package(Qt5Help)
    find_package(Qwt)
endif()

if (UNIX AND NOT APPLE AND NOT ANDROID)
    find_package(Qt5X11Extras)
    find_package(Qt5DBus)
endif ()

# Zeroconf
if (NOT ANDROID)
    include(CMakeModules/zeroconf.cmake)
    add_subdirectory(third_party/aware)
endif()

# Aseba itself
set(LIB_INSTALL_DIR lib CACHE FILEPATH "Where to install libraries")
set(LIB_VERSION_MAJOR 3) # Must be bumped for incompatible ABI changes
set(LIB_VERSION_MINOR 0)
set(LIB_VERSION_PATCH 0)
set(LIB_VERSION_STRING ${LIB_VERSION_MAJOR}.${LIB_VERSION_MINOR}.${LIB_VERSION_PATCH})


if(WIN32)
    add_subdirectory(third_party/bonjour)
    set(ASEBA_SOURCE_DIRECTORY "${PROJECT_SOURCE_DIR}")
    set(CPACK_MODULE_PATH ${PROJECT_SOURCE_DIR}/windows ${CPACK_MODULE_PATH})
    set(CPACK_MONOLITHIC_INSTALL 1)
    configure_file(
        ${PROJECT_SOURCE_DIR}/windows/NSIS.definitions.nsh.in
        ${CMAKE_CURRENT_BINARY_DIR}/resources/NSIS.definitions.nsh
    )
endif()

SET(CPACK_PACKAGE_VERSION_MAJOR "${ASEBA_VERSION_MAJOR}")
SET(CPACK_PACKAGE_VERSION_MINOR "${ASEBA_VERSION_MINOR}")
SET(CPACK_PACKAGE_VERSION_PATCH "${ASEBA_VERSION_PATCH}")
include(CPack)



add_subdirectory(aseba)
add_subdirectory(js)

if(NOT ANDROID)
    # Unit tests
    add_subdirectory(tests)
endif()

# User entries
if (UNIX)
    add_subdirectory(menu)
endif()


if(FLAT_PACK_BUILD)
    SET(MOBSYA_WEBAPPS_ROOT_DIR "share/")
endif()


if(EXISTS ${PROJECT_SOURCE_DIR}/scratch/index.html)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/scratch/ DESTINATION "${MOBSYA_WEBAPPS_ROOT_DIR}scratch")
endif()
if(EXISTS ${PROJECT_SOURCE_DIR}/thymio_blockly/index.html)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/thymio_blockly/ DESTINATION "${MOBSYA_WEBAPPS_ROOT_DIR}thymio_blockly")
endif()


feature_summary( WHAT PACKAGES_FOUND PACKAGES_NOT_FOUND ENABLED_FEATURES DISABLED_FEATURES)






