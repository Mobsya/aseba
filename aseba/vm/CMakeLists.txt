#if (APPLE)

#endif (APPLE)
set (ASEBAVM_SRC
	vm.c
	natives.c
)

if(APPLE)
	add_definitions(-DDISABLE_WEAK_CALLBACKS)
	set(CMAKE_MODULE_LINKER_FLAGS "-Wl,--unresolved-symbols=ignore-in-object-files")
elseif(WIN32)
	set(CMAKE_MODULE_LINKER_FLAGS "/FORCE:UNRESOLVED")
endif()

add_library(asebavm STATIC ${ASEBAVM_SRC})
target_link_libraries(asebavm aseba_conf)
set_target_properties(asebavm PROPERTIES VERSION ${LIB_VERSION_STRING} 
                                        SOVERSION ${LIB_VERSION_MAJOR})

install(TARGETS asebavm 
                LIBRARY DESTINATION ${LIB_INSTALL_DIR} 
                ARCHIVE DESTINATION ${LIB_INSTALL_DIR} 
)

