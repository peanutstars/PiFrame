#
#------------------------------------------------------------------------------

ifndef  BASEDIR
BASEDIR			:=$(shell pwd)/../..
endif
include $(BASEDIR)/Rules.mk

#------------------------------------------------------------------------------

TARGET			:=vmqctl
OBJS			:=vmqctl.o
INSTALLDIR		:=$(TARGETDIR)/system/bin

EXTRA_LFLAGS	+=-lvmq

#------------------------------------------------------------------------------

include $(BASEDIR)/build/Rules.common
