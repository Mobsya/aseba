configure_file("version.cpp.tpl" "version.cpp")


add_library(asebacommon
	utils/FormatableString.cpp
	utils/utils.cpp
	utils/HexFile.cpp
	msg/msg.cpp
	msg/TargetDescription.cpp
	${CMAKE_CURRENT_BINARY_DIR}/version.cpp
)

if(NOT ANDROID)
    target_sources(asebacommon PRIVATE
        msg/NodesManager.cpp
        utils/BootloaderInterface.cpp
    )
endif()

set_target_properties(asebacommon PROPERTIES VERSION ${LIB_VERSION_STRING})
target_link_libraries(asebacommon PUBLIC aseba_conf)

if(ANDROID)
    target_compile_definitions(asebacommon PUBLIC ASEBA_NO_DASHEL)
else()
    target_link_libraries(asebacommon PUBLIC dashel)
endif()

install(TARGETS asebacommon
	LIBRARY DESTINATION ${LIB_INSTALL_DIR}
	ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
)

add_subdirectory(about)

if(HAS_ZEROCONF_SUPPORT)
	add_subdirectory(zeroconf)
endif()
