TEMPLATE = app
TARGET = 
DEPENDPATH += .
INCLUDEPATH += .
QT += script scripttools network
win32: CONFIG += console
mac:CONFIG -= app_bundle
include(../../src/remotetargetdebugger.pri)
SOURCES += main.cpp
