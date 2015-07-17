
ifndef	BASEDIR
BASEDIR			:=$(shell pwd)/..
endif
include $(BASEDIR)/Rules.mk

export BASEDIR MODEL

#------------------------------------------------------------------------------

SCRIPT_FILES := \
				pi-frame.init \
				pi-frame.deinit \
				pfrun


INSTALLDIR		:=$(TARGETDIR)/system/scripts

all: make_source.inform

install:	all
	install -d $(INSTALLDIR)
	install -d $(INSTALLDIR)
	cp -af $(SCRIPT_FILES) $(INSTALLDIR)

clean:
	rm env.pf

distclean:	clean
	rm -rf $(INSTALLDIR)/*

make_source.inform:
	@./genEnvFile.sh


#------------------------------------------------------------------------------

include $(BASEDIR)/build/Rules.common
