TEMPLATE = app

QT += qml
CONFIG += c++14

CONFIG(debug) {
   DEFINES += QML_ROOT_FILE=\"\\\"$$PWD/../qml/thymio-vpl2/main.qml\\\"\"
} else {
   DEFINES += QML_ROOT_FILE=\"\\\"qrc:/thymio-vpl2/main.qml\\\"\"
}

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Default rules for deployment.
include(deployment.pri)
include(../common/common.pri)

SOURCES += \
	$$PWD/main.cpp \
	$$PWD/thymio-vpl2.cpp
