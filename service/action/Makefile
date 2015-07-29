
ifndef  BASEDIR
BASEDIR			:=$(shell pwd)/../..
endif
include $(BASEDIR)/Rules.mk

#------------------------------------------------------------------------------

TARGET			:= svc.action
OBJS			:= event.o \
				   notify.o \
				   config.o \
				   timer.o \
				   main.o

INSTALLDIR		:=$(TARGETDIR)/system/bin

EXTRA_CFLAGS	+=-DSERVICE_NAME=\"$(TARGET)\"

EXTRA_LFLAGS	+=-lpfquery
EXTRA_LFLAGS	+=-lpfconfig
EXTRA_LFLAGS	+=-lvmshm
EXTRA_LFLAGS	+=-lvmq
EXTRA_LFLAGS	+=-lpfutil
EXTRA_LFLAGS	+=-lpfwdt
EXTRA_LFLAGS	+=-lm
EXTRA_LFLAGS	+=-lrt

#------------------------------------------------------------------------------

include $(BASEDIR)/build/Rules.common
