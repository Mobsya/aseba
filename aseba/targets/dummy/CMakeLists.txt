add_executable(asebadummynode dummynode.cpp dummynode_description.c)
target_link_libraries(asebadummynode asebavmbuffer asebavm asebacommon asebazeroconf)
install(TARGETS asebadummynode RUNTIME DESTINATION bin LIBRARY DESTINATION bin)
codesign(asebadummynode)
