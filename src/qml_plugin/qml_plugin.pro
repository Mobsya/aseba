TEMPLATE = lib
CONFIG += qt plugin
QT += qml

DESTDIR  = import/Thymio
TARGET   = qmlthymioplugin
SOURCES += thymioqmlplugin.cpp

pluginfiles.files += qmldir

target.path += import/Thymio
pluginfiles.path = $$DESTDIR

INSTALLS += target pluginfiles
CONFIG += install_ok
COPIES += pluginfiles


# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Default rules for deployment.
include(../app/deployment.pri)
include(../common/common.pri)
