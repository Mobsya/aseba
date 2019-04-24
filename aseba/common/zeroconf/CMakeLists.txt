add_library(asebazeroconf STATIC
	zeroconf.cpp
	txtrecord.cpp
	target.cpp
	zeroconf-thread.cpp
	zeroconf-dashelhub.cpp
)
set_target_properties(asebazeroconf PROPERTIES VERSION ${LIB_VERSION_STRING})
target_link_libraries(asebazeroconf PUBLIC asebacommon zeroconf)