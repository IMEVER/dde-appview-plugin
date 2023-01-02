#-------------------------------------------------
#
# Project created by QtCreator 2016-12-21T13:15:17
#
#-------------------------------------------------

QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = dde-appview-plugin
TEMPLATE = lib
CONFIG += c++11 plugin link_pkgconfig
PKGCONFIG += dtkwidget dtkgui

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
    appplugin.cpp \
    viewplugin.cpp \
    appview.cpp \
    desktoplistview.cpp

HEADERS += \
    appplugin.h \
    viewplugin.h \
    appview.h \
    desktoplistview.h
DISTFILES += \
    appView.json

unix {
    target.path = $$PLUGINDIR/views
    INSTALLS += target
}

RESOURCES += \
    appview.qrc
