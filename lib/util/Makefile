#
#------------------------------------------------------------------------------

ifndef	BASEDIR
BASEDIR			:=$(shell pwd)/../..
endif
include $(BASEDIR)/Rules.mk

#------------------------------------------------------------------------------

TARGET			:= libpfutil.so
OBJS			:= utilDebug.o \
				   utilAvcrc.o \
				   utilFile.o \
				   utilNet.o \
				   utilSignal.o \
				   utilSys.o

INSTALLDIR		:=$(TARGETDIR)/system/lib

EXTRA_CFLAGS	+=-fPIC -fpic
EXTRA_LFLAGS	+=-shared -Wl,-soname,$(TARGET)

#------------------------------------------------------------------------------

include $(BASEDIR)/build/Rules.common
