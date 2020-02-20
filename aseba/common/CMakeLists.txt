configure_file("version.cpp.tpl" "version.cpp")


add_library(asebacommon STATIC
	utils/FormatableString.cpp
	utils/utils.cpp
	msg/msg.cpp
	msg/TargetDescription.cpp
	${CMAKE_CURRENT_BINARY_DIR}/version.cpp
)

set_target_properties(asebacommon PROPERTIES VERSION ${LIB_VERSION_STRING})
target_link_libraries(asebacommon PUBLIC aseba_conf)

if(NOT ANDROID)
    add_subdirectory(about)
endif()