
ifndef	BASEDIR
BASEDIR			:=$(shell pwd)/../..
endif
include $(BASEDIR)/Rules.mk

#------------------------------------------------------------------------------

include $(BASEDIR)/build/Rules.common

INSTALLDIR		:=$(TARGETDIR)/system/bin
INSTALLTARGET	:= \
				   iperf \

#------------------------------------------------------------------------------

.PHONY: all clean install distclean

all:

clean:

distclean:
	@ls | grep -v Makefile | xargs rm -rf

install:
	@install -d $(INSTALLDIR)
	@cp -a $(INSTALLTARGET) $(INSTALLDIR)

