cmake_minimum_required(VERSION 3.0)

find_program(NPM_EXECUTABLE NAMES npm)
if (NOT NPM_EXECUTABLE)
    message(FATAL_ERROR "Could not find npm")
endif()

add_custom_target(thymio-js-api ALL DEPENDS package.json  package-lock.json ${CMAKE_CURRENT_SOURCE_DIR}/thymio_generated.js thymio.js ${CMAKE_CURRENT_BINARY_DIR}/mobsya-thymio-api.tgz)


add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/package-lock.json
    COMMAND ${NPM_EXECUTABLE} install
    COMMENT "thymio-js-api dependencies"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    DEPENDS package.json
)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/thymio_generated.js
  COMMAND flatc --js --gen-mutable --es6-js-export --gen-object-api -o "${CMAKE_CURRENT_SOURCE_DIR}" ${CMAKE_CURRENT_SOURCE_DIR}/../aseba/flatbuffers/thymio.fbs
  DEPENDS flatc thymio-flatbuffers ${CMAKE_CURRENT_SOURCE_DIR}/../aseba/flatbuffers/thymio.fbs
  COMMENT "Generating ${CMAKE_CURRENT_SOURCE_DIR}/thymio_generated.js"
)

add_custom_command(
    OUTPUT mobsya-thymio-api.tgz
    COMMAND ${NPM_EXECUTABLE} pack ${CMAKE_CURRENT_SOURCE_DIR} && mv mobsya-thymio-api-*.tgz mobsya-thymio-api.tgz
    COMMENT "Building mobsya-thymio-api"
    DEPENDS thymio-flatbuffers package.json package-lock.json ${CMAKE_CURRENT_SOURCE_DIR}/thymio_generated.js thymio.js ${timestamp_file}
)
