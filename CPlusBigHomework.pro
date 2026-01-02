QT       += core gui widgets printsupport
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = LidarVis
TEMPLATE = app
DEFINES += QT_DEPRECATED_WARNINGS

# 解决 "file too big" 问题
QMAKE_CXXFLAGS += -Wa,-mbig-obj

CONFIG += c++17

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    datamanager.cpp \
    ppiwidget.cpp \
    qcustomplot.cpp

HEADERS += \
    mainwindow.h \
    datamanager.h \
    datatypes.h \
    ppiwidget.h \
    qcustomplot.h

win32:DEFINES += _USE_MATH_DEFINES
