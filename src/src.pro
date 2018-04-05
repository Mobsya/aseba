TEMPLATE = app

QT += qml
CONFIG += c++14

SOURCES += main.cpp

include(../third_party/libusb/libusb_lib.pri)
include(thymio-vpl2.pri)
include(../third_party/qtzeroconf/qtzeroconf_lib.pri)

CONFIG(debug) {
   include(../third_party/qmllive/src/src.pri)
   DEFINES += QMLLIVE_ENABLED
   DEFINES += QML_ROOT_FILE=\"\\\"$$PWD/qml/thymio-vpl2/main.qml\\\"\"
} else {
   DEFINES += QML_ROOT_FILE=\"\\\"qrc:/thymio-vpl2/main.qml\\\"\"
}

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Default rules for deployment.
include(deployment.pri)

DISTFILES += \
    ../android/AndroidManifest.xml \
    ../android/res/values/libs.xml \
    ../android/res/xml/device_filter.xml \
    ../android/build.gradle \
    ../android/settings.gradle \
    ../android/gradle/**/*

DISTFILES += $$files(../android/*.java, true)


ANDROID_PACKAGE_SOURCE_DIR = $$PWD/../android
