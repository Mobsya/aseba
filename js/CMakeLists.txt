cmake_minimum_required(VERSION 3.0)

find_program(NPM_EXECUTABLE NAMES npm NO_CMAKE_FIND_ROOT_PATH)

if (NOT NPM_EXECUTABLE)
    message(FATAL_ERROR "Could not find npm")
endif()

set(timestamp_file ${CMAKE_CURRENT_BINARY_DIR}/timestamp.cmake)
set(timestamp_file2 ${CMAKE_CURRENT_BINARY_DIR}/timestamp2.cmake)

add_custom_target(thymio-js-api ALL
    DEPENDS package.json thymio.js ${timestamp_file2}
    SOURCES package.json thymio.js thymio_generated.js)

set_property(TARGET thymio-js-api PROPERTY DEPENDS ${timestamp_file2})

add_custom_command(
    OUTPUT  ${timestamp_file}
    COMMAND ${NPM_EXECUTABLE} install
    COMMAND ${CMAKE_COMMAND} -E touch ${timestamp_file}
    COMMENT "thymio-js-api dependencies"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS package.json thymio.js
)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/thymio_generated.js ${timestamp_file2}
  COMMAND flatc --js --gen-mutable --es6-js-export --gen-object-api -o "${CMAKE_CURRENT_SOURCE_DIR}" ${CMAKE_CURRENT_SOURCE_DIR}/../aseba/flatbuffers/thymio.fbs
  COMMAND ${CMAKE_COMMAND} -E touch ${timestamp_file2}
  DEPENDS flatc thymio-flatbuffers ${CMAKE_CURRENT_SOURCE_DIR}/../aseba/flatbuffers/thymio.fbs ${timestamp_file}
  COMMENT "Generating ${CMAKE_CURRENT_SOURCE_DIR}/thymio_generated.js"
)
