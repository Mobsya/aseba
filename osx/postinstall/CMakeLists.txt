find_package (Python3 COMPONENTS Interpreter REQUIRED)
install(CODE " execute_process( COMMAND \"${Python3_EXECUTABLE}\" \"${CMAKE_CURRENT_SOURCE_DIR}/create_bundle.py\" \"${CMAKE_BINARY_DIR}\")")

#CMAKE_BINARY_DIR