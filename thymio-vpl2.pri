ASEBA_SOURCES = \
	$$PWD/enki/enki/Types.cpp \
	$$PWD/enki/enki/Geometry.cpp \
	$$PWD/enki/enki/PhysicalEngine.cpp \
	$$PWD/enki/enki/BluetoothBase.cpp \
	$$PWD/enki/enki/interactions/GroundSensor.cpp \
	$$PWD/enki/enki/interactions/IRSensor.cpp \
	$$PWD/enki/enki/robots/DifferentialWheeled.cpp \
	$$PWD/enki/enki/robots/thymio2/Thymio2.cpp \
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
	$$PWD/aseba/compiler/tree-emit.cpp \
	$$PWD/aseba/vm/vm.c \
	$$PWD/aseba/vm/natives.c \
	$$PWD/aseba/transport/buffer/vm-buffer.c \
	$$PWD/aseba/targets/playground/EnkiGlue.cpp \
	$$PWD/aseba/targets/playground/AsebaGlue.cpp \
	$$PWD/aseba/targets/playground/DirectAsebaGlue.cpp \
	$$PWD/aseba/targets/playground/Thymio2.cpp \
	$$PWD/aseba/targets/playground/Thymio2-natives.cpp \
	$$PWD/aseba/targets/playground/Thymio2-descriptions.c
ASEBA_DEFINES =
macx {
	ASEBA_DEFINES += DISABLE_WEAK_CALLBACKS
}
ASEBA_LIBS =
win32 {
	ASEBA_SOURCES += $$PWD/dashel/dashel/dashel-win32.cpp
} else {
	ASEBA_SOURCES += $$PWD/dashel/dashel/dashel-posix.cpp
        mac {
		ASEBA_SOURCES += $$PWD/dashel/dashel/poll_emu.c
		ASEBA_LIBS += -framework CoreFoundation
	}
	linux:!android {
		ASEBA_DEFINES += USE_LIBUDEV
		ASEBA_LIBS += -ludev
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
ASEBA_INCLUDE = $$PWD/dashel $$PWD/enki $$PWD
!msvc {
	ASEBA_CXXFLAGS = -Wno-unused-parameter -Wno-deprecated-declarations
}

QMAKE_CXXFLAGS += $$ASEBA_CXXFLAGS
DEFINES += $$ASEBA_DEFINES
CONFIG += c++14 object_parallel_to_source
HEADERS += $$PWD/aseba.h \
	$$PWD/simulator.h
SOURCES += \
	$$ASEBA_SOURCES \
	$$PWD/aseba.cpp \
	$$PWD/thymio-vpl2.cpp \
	$$PWD/simulator.cpp
lupdate_only{
SOURCES = \
	$$PWD/*.qml \
	$$PWD/blocks/*.qml \
	$$PWD/blocks/widgets/*.qml
}
TRANSLATIONS += translations/thymio-vpl2_fr.ts translations/thymio-vpl2_de.ts
QT += quick svg xml
RESOURCES += $$PWD/thymio-vpl2.qrc
DEPENDPATH += $$ASEBA_INCLUDE
INCLUDEPATH += $$ASEBA_INCLUDE
LIBS += $$ASEBA_LIBS
