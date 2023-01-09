#-------------------------------------------------
#
# Project created by QtCreator 2016-12-21T13:15:17
#
#-------------------------------------------------

QT       += core gui concurrent dbus
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = dde-appview-plugin
TEMPLATE = lib
CONFIG += c++11 plugin link_pkgconfig
PKGCONFIG += dtkwidget dtkgui dframeworkdbus

include(../../common/common.pri)
#include(../plugininterfaces/plugininterfaces.pri)

#安全加固
QMAKE_CXXFLAGS += -fstack-protector-all
QMAKE_LFLAGS += -z now -fPIC
isEqual(ARCH, mips64) | isEqual(ARCH, mips32){
    QMAKE_LFLAGS += -z noexecstack -z relro
}

DESTDIR = ../view

SOURCES += \
    src/appplugin.cpp \
    src/desktopfilemodel.cpp \
    src/launcherdbusinterface.cpp \
    src/packageinfoview.cpp \
    src/packagetreeview.cpp \
    src/viewplugin.cpp \
    src/appview.cpp \
    src/desktoplistview.cpp

HEADERS += \
    src/appplugin.h \
    src/desktopfilemodel.h \
    src/launcherdbusinterface.h \
    src/packageinfoview.h \
    src/packagetreeview.h \
    src/viewplugin.h \
    src/appview.h \
    src/desktoplistview.h
DISTFILES += \
    resource/appView.json

unix {
    target.path = $$PLUGINDIR/views
    INSTALLS += target
}

RESOURCES += \
    resource/appview.qrc
