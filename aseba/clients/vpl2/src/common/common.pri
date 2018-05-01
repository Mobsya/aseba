include(../../../../../third_party/qtzeroconf/qtzeroconf_lib.pri)
LIBS += -L../../../../..


ASEBA_DIR=$$PWD/../../../..

ASEBA_SOURCES = \
    $$ASEBA_DIR/common/utils/FormatableString.cpp \
    $$ASEBA_DIR/common/utils/utils.cpp \
    $$ASEBA_DIR/common/utils/HexFile.cpp \
    $$ASEBA_DIR/common/msg/msg.cpp \
    $$ASEBA_DIR/common/msg/TargetDescription.cpp \
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

    #$$ASEBA_DIR/vm/vm.c \
    #$$ASEBA_DIR/vm/natives.c \
    #$$ASEBA_DIR/transport/buffer/vm-buffer.c

ASEBA_DEFINES =
macx {
    ASEBA_DEFINES += DISABLE_WEAK_CALLBACKS
}
ASEBA_LIBS =
win32 {
	ASEBA_LIBS += -lwinspool -lws2_32 -lSetupapi
} else {
		mac {
		ASEBA_LIBS += -framework CoreFoundation
	}
	linux:!android {
		ASEBA_DEFINES += USE_LIBUDEV
		ASEBA_LIBS += -ludev
	}
}

ASEBA_INCLUDE = $$ASEBA_DIR $$ASEBA_DIR/..


SOURCES += \
    $$ASEBA_SOURCES \
    $$PWD/aseba.cpp \
    $$PWD/thymio/ThymioManager.cpp \
    $$PWD/thymio/NetworkDeviceProber.cpp \
    $$PWD/thymio/DeviceQtConnection.cpp \
    $$PWD/thymio/ThymioProviderInfo.cpp \
    $$PWD/thymio/ThymioListModel.cpp


HEADERS += \
    $$PWD/aseba.h \
    $$PWD/thymio/ThymioManager.h \
    $$PWD/thymio/NetworkDeviceProber.h \
    $$PWD/thymio/DeviceQtConnection.h \
    $$PWD/thymio/ThymioProviderInfo.h \
    $$PWD/thymio/ThymioListModel.h

!android:!ios {
   SOURCES += $$PWD/thymio/UsbSerialDeviceProber.cpp
   HEADERS += $$PWD/thymio/UsbSerialDeviceProber.h
   QT += serialport
}

android {
   QT += androidextras
}

!msvc {
    QMAKE_CXXFLAGS += -Wno-unused-parameter -Wno-deprecated-declarations
}

DEFINES += $$ASEBA_DEFINES ASEBA_NO_DASHEL
DEFINES += QZEROCONF_STATIC
CONFIG += c++14 object_parallel_to_source

HEADERS += $$PWD/aseba.h

lupdate_only{
SOURCES += \
  $$PWD/../qml/thymio-vpl2/*.qml \
  $$PWD/../qml/thymio-vpl2/blocks/*.qml \
  $$PWD/../qml/thymio-vpl2/blocks/widgets/*.qml
}

QT += quick svg xml
DEPENDPATH += $$ASEBA_INCLUDE $$PWD
INCLUDEPATH += $$ASEBA_INCLUDE $$PWD
LIBS += $$ASEBA_LIBS
