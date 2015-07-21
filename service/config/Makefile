
ifndef  BASEDIR
BASEDIR			:=$(shell pwd)/../..
endif
include $(BASEDIR)/Rules.mk

#------------------------------------------------------------------------------

TARGET			:= svc.config
OBJS			:= config.o \
				   event.o \
				   notify.o


#				   main.o \
				   config-system.o \
				   config-network.o \
				   config-util.o 

INSTALLDIR		:=$(TARGETDIR)/system/bin

EXTRA_CFLAGS	+=-DSERVICE_NAME=\"$(TARGET)\"

EXTRA_LFLAGS	+=-lvmshm
EXTRA_LFLAGS	+=-lvmq
EXTRA_LFLAGS	+=-lcjson
EXTRA_LFLAGS	+=-lpfutil
EXTRA_LFLAGS	+=-lpfwdt
EXTRA_LFLAGS	+=-lm
EXTRA_LFLAGS	+=-lrt

#------------------------------------------------------------------------------

include $(BASEDIR)/build/Rules.common
