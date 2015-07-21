#
#------------------------------------------------------------------------------

ifndef	BASEDIR
BASEDIR			:=$(shell pwd)/../..
endif
include $(BASEDIR)/Rules.mk

#------------------------------------------------------------------------------

TARGET			:=libpfwdt.so
OBJS			:=pfwdt.o
INSTALLDIR		:=$(TARGETDIR)/system/lib

EXTRA_CFLAGS	+=-fPIC -fpic
EXTRA_LFLAGS	+=-shared -Wl,-soname,$(TARGET)

#------------------------------------------------------------------------------

include $(BASEDIR)/build/Rules.common
