
ifndef	BASEDIR
BASEDIR			:=$(shell pwd)/../..
endif
include $(BASEDIR)/Rules.mk

#------------------------------------------------------------------------------

include $(BASEDIR)/build/Rules.common

INSTALLDIR		:=$(TARGETDIR)/system/lib

#------------------------------------------------------------------------------

.PHONY: all clean install distclean

all:

clean:

distclean:
	./distclean.sh .

install:
	@install -d $(INSTALLDIR)
	@cp -a *.so* $(INSTALLDIR)
