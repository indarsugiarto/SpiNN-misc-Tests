TARGET     = qReadT
QT        += core
QT        += network
QT        -= gui
TEMPLATE   = app
CONFIG    += console
CONFIG    -= app_bundle

SOURCES   += main.cpp \
             ctask.cpp \
             tempReader.cpp

HEADERS += ctask.h \
           tempReader.h
