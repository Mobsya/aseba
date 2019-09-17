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
