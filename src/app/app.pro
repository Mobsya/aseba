TEMPLATE = app
TARGET = thymiovpl

QT += qml
CONFIG += c++14

CONFIG(debug):!android{
   DEFINES += QML_ROOT_FILE=\"\\\"$$PWD/../qml/thymio-vpl2/main.qml\\\"\"
} else {
   DEFINES += QML_ROOT_FILE=\"\\\"qrc:/thymio-vpl2/main.qml\\\"\"
}

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH += $$PWD/../qml
QML_ROOT_PATH   += $$PWD/../qml

# Default rules for deployment.
include(deployment.pri)
include(../common/common.pri)

SOURCES += \
	$$PWD/main.cpp \
	$$PWD/thymio-vpl2.cpp \
	liveqmlreloadingengine.cpp

HEADERS += \
	liveqmlreloadingengine.h

QT += quick
TRANSLATIONS += ../translations/thymio-vpl2_fr.ts ../translations/thymio-vpl2_de.ts
RESOURCES += $$PWD/../qml/thymio-vpl2.qrc
DISTFILES += \
    ../../android/AndroidManifest.xml \
    ../../android/res/values/libs.xml \
    ../../android/res/xml/device_filter.xml \
    ../../android/build.gradle \
    ../../android/settings.gradle \
    ../../android/gradle/**/* \
    ../../android/gradlew \
    ../../android/gradlew.bat

DISTFILES += $$files(../../android/*.java, true)


ANDROID_PACKAGE_SOURCE_DIR = $$PWD/../../android

QMAKE_CXXFLAGS_DEBUG = -g3 -O0
