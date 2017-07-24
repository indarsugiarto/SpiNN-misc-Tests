TEMPLATE = app
CONFIG  += console
CONFIG  -= app_bundle
CONFIG  -= qt

SOURCES += profiler_main.c \
           profiler_pll.c \
	   profiler_temp.c \
	   profiler_cpuload.c \
	   profiler_events.c \
	   profiler_util.c

HEADERS += profiler.h \

DISTFILES +=     Makefile \
	   compile

INCLUDEPATH += /opt/spinnaker_tools_3.1.0/include
