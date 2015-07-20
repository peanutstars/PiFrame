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


EXTRA_INSTALL	:= 1
EXTRA_FILES		:= vmshmTest

#------------------------------------------------------------------------------

include $(BASEDIR)/build/Rules.common

extra_install:
	install $(EXTRA_FILES) $(INSTALLDIR)

