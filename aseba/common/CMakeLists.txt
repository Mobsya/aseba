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

set (ASEBACORE_HDR_UTILS
	utils/utils.h
	utils/FormatableString.h
)
set (ASEBACORE_HDR_MSG
	msg/msg.h
	msg/NodesManager.h
	msg/TargetDescription.h
)
set (ASEBACORE_HDR_COMMON
	consts.h
	types.h
	productids.h
)
install(FILES ${ASEBACORE_HDR_UTILS}
	DESTINATION include/aseba/common/utils
)
install(FILES ${ASEBACORE_HDR_MSG}
	DESTINATION include/aseba/common/msg
)
install(FILES ${ASEBACORE_HDR_COMMON}
	DESTINATION include/aseba/common
)

add_subdirectory(about)

if(HAS_ZEROCONF_SUPPORT)
	add_subdirectory(zeroconf)
endif()
