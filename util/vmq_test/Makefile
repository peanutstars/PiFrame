#
#------------------------------------------------------------------------------

ifndef  BASEDIR
BASEDIR			:=$(shell pwd)/../..
endif
include $(BASEDIR)/Rules.mk

#------------------------------------------------------------------------------

TARGET			:= vmq_test
OBJS			:= event.o \
				   notify.o \
				   main.o

INSTALLDIR		:=$(TARGETDIR)/system/bin

EXTRA_LFLAGS    +=-lvmshm
EXTRA_LFLAGS	+=-lvmq
EXTRA_LFLAGS	+=-lpthread

EXTRA_INSTALL	:= 1
EXTRA_FILES		:= vmqTest

#------------------------------------------------------------------------------

include $(BASEDIR)/build/Rules.common

extra_install:
	install $(EXTRA_FILES) $(INSTALLDIR)


