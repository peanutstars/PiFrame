
ifndef  BASEDIR
BASEDIR			:=$(shell pwd)/../..
endif
include $(BASEDIR)/Rules.mk

#------------------------------------------------------------------------------

TARGET			:= svc.watchdog
OBJS			:= \
				   event.o \
				   eventWD.o \
				   watchdog.o \
				   main.o
				   


INSTALLDIR		:=$(TARGETDIR)/system/bin

EXTRA_CFLAGS	+=-DSERVICE_NAME=\"$(TARGET)\"

EXTRA_LFLAGS	+=-lvmshm
EXTRA_LFLAGS	+=-lvmq
EXTRA_LFLAGS	+=-lpfutil
EXTRA_LFLAGS	+=-lpthread
EXTRA_LFLAGS	+=-lrt


#------------------------------------------------------------------------------

include $(BASEDIR)/build/Rules.common
