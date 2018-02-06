ASEBA_DIR=$$PWD/aseba/aseba
!exists($${ASEBA_DIR}/CMakeLists.txt) {
	ASEBA_DIR=$$PWD/aseba
}


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
	$$ASEBA_DIR/common/utils/FormatableString.cpp \
	$$ASEBA_DIR/common/utils/utils.cpp \
	$$ASEBA_DIR/common/utils/HexFile.cpp \
	$$ASEBA_DIR/common/utils/BootloaderInterface.cpp \
	$$ASEBA_DIR/common/msg/msg.cpp \
	$$ASEBA_DIR/common/msg/NodesManager.cpp \
	$$ASEBA_DIR/compiler/compiler.cpp \
	$$ASEBA_DIR/compiler/errors.cpp \
	$$ASEBA_DIR/compiler/identifier-lookup.cpp \
	$$ASEBA_DIR/compiler/lexer.cpp \
	$$ASEBA_DIR/compiler/parser.cpp \
	$$ASEBA_DIR/compiler/analysis.cpp \
	$$ASEBA_DIR/compiler/tree-build.cpp \
	$$ASEBA_DIR/compiler/tree-expand.cpp \
	$$ASEBA_DIR/compiler/tree-dump.cpp \
	$$ASEBA_DIR/compiler/tree-typecheck.cpp \
	$$ASEBA_DIR/compiler/tree-optimize.cpp \
	$$ASEBA_DIR/compiler/tree-emit.cpp \
	$$ASEBA_DIR/vm/vm.c \
	$$ASEBA_DIR/vm/natives.c \
	$$ASEBA_DIR/transport/buffer/vm-buffer.c \
	$$ASEBA_DIR/targets/playground/EnkiGlue.cpp \
	$$ASEBA_DIR/targets/playground/AsebaGlue.cpp \
	$$ASEBA_DIR/targets/playground/DirectAsebaGlue.cpp \
	$$ASEBA_DIR/targets/playground/robots/thymio2/Thymio2.cpp \
	$$ASEBA_DIR/targets/playground/robots/thymio2/Thymio2-natives.cpp \
	$$ASEBA_DIR/targets/playground/robots/thymio2/Thymio2-descriptions.c
ASEBA_DEFINES =
macx {
	ASEBA_DEFINES += DISABLE_WEAK_CALLBACKS
}
ASEBA_LIBS =
win32 {
	ASEBA_SOURCES += $$PWD/dashel/dashel/dashel-win32.cpp
	ASEBA_LIBS += -lwinspool -lws2_32 -lSetupapi
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
	ASEBA_SOURCES += $$ASEBA_DIR/transport/dashel_plugins/android.cpp
	aseba_android.files = $$PWD/android/*
	aseba_android.path = /
	INSTALLS += aseba_android
} else {
	ASEBA_SOURCES += $$ASEBA_DIR/transport/dashel_plugins/none.cpp
}
ASEBA_INCLUDE = $$PWD/dashel $$PWD/enki $$PWD/aseba $$PWD/aseba/aseba
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
