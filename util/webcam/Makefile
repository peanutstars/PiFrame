#
#------------------------------------------------------------------------------

ifndef  BASEDIR
BASEDIR			:=$(shell pwd)/../..
endif
include $(BASEDIR)/Rules.mk

#------------------------------------------------------------------------------

TARGET			:= capture
OBJS			:= capture.o

INSTALLDIR		:=$(TARGETDIR)/system/bin

EXTRA_CFLAGS	+=-I$(VM_ROOT_DIR)/SDK/XCMSDK/include

#EXTRA_LFLAGS	+=-lpfconfig
#EXTRA_LFLAGS    +=-lvmshm
#EXTRA_LFLAGS	+=-lvmq
#EXTRA_LFLAGS	+=-lpfquery
#EXTRA_LFLAGS	+=-lpfutil
#EXTRA_LFLAGS	+=-lpthread
#EXTRA_LFLAGS	+=-lrt

#------------------------------------------------------------------------------

include $(BASEDIR)/build/Rules.common
