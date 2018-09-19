set (ASEBACOMPILER_SRC
	compiler.cpp
	errors.cpp
	identifier-lookup.cpp
	lexer.cpp
	parser.cpp
	analysis.cpp
	tree-build.cpp
	tree-expand.cpp
	tree-dump.cpp
	tree-typecheck.cpp
	tree-optimize.cpp
	tree-emit.cpp
)
add_library(asebacompiler STATIC ${ASEBACOMPILER_SRC})
target_link_libraries(asebacompiler asebacommon)
