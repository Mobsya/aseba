cmake_minimum_required(VERSION 3.0)

find_program(NPM_EXECUTABLE NAMES npm NO_CMAKE_FIND_ROOT_PATH)

if (NPM_EXECUTABLE)
    set(timestamp_file ${CMAKE_CURRENT_BINARY_DIR}/timestamp.cmake)
    set(timestamp_file2 ${CMAKE_CURRENT_BINARY_DIR}/timestamp2.cmake)
    set(timestamp_file3 ${CMAKE_CURRENT_BINARY_DIR}/timestamp3.cmake)
    set(tarball_file  ${CMAKE_CURRENT_BINARY_DIR}/mobsya-association-thymio-api.tar.gz)

    add_custom_target(thymio-js-api ALL
        DEPENDS package-lock.json package.json src/thymio.ts ${timestamp_file2} ${timestamp_file3} ${tarball_file}
        SOURCES package-lock.json package.json src/thymio.ts src/thymio_generated.ts)

    get_filename_component(ABSOLUTE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} ABSOLUTE)

    set_property(TARGET thymio-js-api PROPERTY DEPENDS ${timestamp_file3})
    set_property(TARGET thymio-js-api PROPERTY TARBALL ${tarball_file})

    add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/src/thymio_generated.ts ${timestamp_file2}
    COMMAND flatc --ts --no-fb-import --gen-mutable --gen-object-api -o "${CMAKE_CURRENT_SOURCE_DIR}/src/" ${CMAKE_CURRENT_SOURCE_DIR}/../aseba/flatbuffers/thymio.fbs
    COMMAND ${CMAKE_COMMAND} -E touch ${timestamp_file2}
    DEPENDS flatc thymio-flatbuffers ${CMAKE_CURRENT_SOURCE_DIR}/../aseba/flatbuffers/thymio.fbs
    COMMENT "Generating ${CMAKE_CURRENT_SOURCE_DIR}/src/thymio_generated.ts"
    )

    add_custom_command(
        OUTPUT  ${timestamp_file3} ${tarball_file} ${CMAKE_CURRENT_SOURCE_DIR}/package-lock.json
        COMMAND ${NPM_EXECUTABLE} install
        COMMAND ${NPM_EXECUTABLE} run gen-tarball --args "${tarball_file}"
        COMMAND ${CMAKE_COMMAND} -E touch ${timestamp_file3}
        COMMENT "build thymio-js-api"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        DEPENDS package.json src/thymio.ts src/thymio_generated.ts ${timestamp_file2}
    )
endif()