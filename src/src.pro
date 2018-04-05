TEMPLATE = app

QT += qml
CONFIG += c++14

SOURCES += main.cpp

include(../third_party/libusb/libusb_lib.pri)
include(thymio-vpl2.pri)
include(../third_party/qtzeroconf/qtzeroconf_lib.pri)

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
