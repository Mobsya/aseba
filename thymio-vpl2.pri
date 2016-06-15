ASEBA_SOURCES = \
	$$PWD/dashel/dashel/dashel-common.cpp \
	$$PWD/aseba/common/utils/FormatableString.cpp \
	$$PWD/aseba/common/utils/utils.cpp \
	$$PWD/aseba/common/utils/HexFile.cpp \
	$$PWD/aseba/common/utils/BootloaderInterface.cpp \
	$$PWD/aseba/common/msg/msg.cpp \
	$$PWD/aseba/common/msg/NodesManager.cpp \
	$$PWD/aseba/compiler/compiler.cpp \
	$$PWD/aseba/compiler/errors.cpp \
	$$PWD/aseba/compiler/identifier-lookup.cpp \
	$$PWD/aseba/compiler/lexer.cpp \
	$$PWD/aseba/compiler/parser.cpp \
	$$PWD/aseba/compiler/analysis.cpp \
	$$PWD/aseba/compiler/tree-build.cpp \
	$$PWD/aseba/compiler/tree-expand.cpp \
	$$PWD/aseba/compiler/tree-dump.cpp \
	$$PWD/aseba/compiler/tree-typecheck.cpp \
	$$PWD/aseba/compiler/tree-optimize.cpp \
	$$PWD/aseba/compiler/tree-emit.cpp
win32 {
	ASEBA_SOURCES += $$PWD/dashel/dashel/dashel-win32.cpp
} else {
	ASEBA_SOURCES += $$PWD/dashel/dashel/dashel-posix.cpp
	macx {
		ASEBA_SOURCES += $$PWD/dashel/dashel/poll_emu.c
		ASEBA_LIBS = -framework CoreFoundation
	}
}
android {
	ASEBA_SOURCES += $$PWD/aseba/transport/dashel_plugins/android.cpp
	aseba_android.files = $$PWD/android/*
	aseba_android.path = /
	INSTALLS += aseba_android
} else {
	ASEBA_SOURCES += $$PWD/aseba/transport/dashel_plugins/none.cpp
}
ASEBA_INCLUDE = $$PWD/dashel $$PWD
ASEBA_CXXFLAGS = -Wno-unused-parameter -Wno-deprecated-declarations

QMAKE_CXXFLAGS += $$ASEBA_CXXFLAGS
CONFIG += c++11
HEADERS += $$PWD/aseba.h
SOURCES += $$ASEBA_SOURCES $$PWD/aseba.cpp
QT += quick svg xml
RESOURCES += $$PWD/thymio-vpl2.qrc
DEPENDPATH += $$ASEBA_INCLUDE
INCLUDEPATH += $$ASEBA_INCLUDE
LIBS += $$ASEBA_LIBS
