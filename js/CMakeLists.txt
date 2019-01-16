cmake_minimum_required(VERSION 3.0)

find_program(NPM_EXECUTABLE NAMES npm NO_CMAKE_FIND_ROOT_PATH)

if (NOT NPM_EXECUTABLE)
    message(FATAL_ERROR "Could not find npm")
endif()

set(timestamp_file ${CMAKE_CURRENT_BINARY_DIR}/timestamp.cmake)
set(timestamp_file2 ${CMAKE_CURRENT_BINARY_DIR}/timestamp2.cmake)

add_custom_target(thymio-js-api ALL
    DEPENDS package.json src/thymio.ts ${timestamp_file} ${timestamp_file2}
    SOURCES package.json src/thymio.ts dist/thymio_generated.ts)

get_filename_component(ABSOLUTE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} ABSOLUTE)

set_property(TARGET thymio-js-api PROPERTY DEPENDS ${timestamp_file2})
set_property(TARGET thymio-js-api PROPERTY ABSOLUTE_SOURCE_DIR ${ABSOLUTE_SOURCE_DIR})

add_custom_command(
    OUTPUT  ${timestamp_file}
    COMMAND ${NPM_EXECUTABLE} install
    COMMAND ${CMAKE_COMMAND} -E touch ${timestamp_file}
    COMMENT "thymio-js-api dependencies"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS package.json src/thymio.ts
)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/dist/thymio_generated.ts ${timestamp_file2}
  COMMAND flatc --ts --no-fb-import --gen-mutable --gen-object-api -o "${CMAKE_CURRENT_SOURCE_DIR}/dist/" ${CMAKE_CURRENT_SOURCE_DIR}/../aseba/flatbuffers/thymio.fbs
  COMMAND ${CMAKE_COMMAND} -E touch ${timestamp_file2}
  DEPENDS flatc thymio-flatbuffers ${CMAKE_CURRENT_SOURCE_DIR}/../aseba/flatbuffers/thymio.fbs ${timestamp_file}
  COMMENT "Generating ${CMAKE_CURRENT_SOURCE_DIR}/dist/thymio_generated.ts"
)