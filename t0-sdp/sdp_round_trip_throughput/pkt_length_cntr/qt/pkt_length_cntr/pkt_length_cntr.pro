QT += core network
QT -= gui

TARGET = pkt_length_cntr
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    ctask.cpp

HEADERS += \
    ctask.h

