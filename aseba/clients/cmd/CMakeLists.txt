add_executable(asebacmd
	cmd.cpp
)
target_link_libraries(asebacmd asebacommon asebadashelplugins)
install(TARGETS asebacmd RUNTIME
	DESTINATION bin
)
set_target_properties(asebacmd PROPERTIES MACOSX_PACKAGE_LOCATION MacOsX)

codesign(asebacmd)
