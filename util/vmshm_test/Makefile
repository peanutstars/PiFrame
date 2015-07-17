#
#------------------------------------------------------------------------------

ifndef  BASEDIR
BASEDIR			:=$(shell pwd)/../..
endif
include $(BASEDIR)/Rules.mk

#------------------------------------------------------------------------------

TARGET			:=vmshm_test
OBJS			:= config.o \
				   vmshm_test.o


INSTALLDIR		:=$(TARGETDIR)/system/bin

EXTRA_LFLAGS    +=-lvmshm
EXTRA_LFLAGS	+=-lpthread

#------------------------------------------------------------------------------

include $(BASEDIR)/build/Rules.common
