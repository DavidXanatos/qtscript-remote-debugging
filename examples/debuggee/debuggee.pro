TEMPLATE = app
TARGET = 
DEPENDPATH += .
INCLUDEPATH += .
QT += network script scripttools
win32: CONFIG += console
mac:CONFIG -= app_bundle
include(../../src/debuggerengine.pri)
SOURCES += main.cpp
