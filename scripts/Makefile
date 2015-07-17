
ifndef	BASEDIR
BASEDIR			:=$(shell pwd)/..
endif
include $(BASEDIR)/Rules.mk

export BASEDIR MODEL

#------------------------------------------------------------------------------

PF_ENV_FILE	:= pf-env

SCRIPT_FILES := \
				pi-frame.init \
				pi-frame.deinit \
				pf-run \
				$(PF_ENV_FILE)


INSTALLDIR		:=$(TARGETDIR)/system/scripts

all: make_source.inform

install:	all
	install -d $(INSTALLDIR)
	install -d $(INSTALLDIR)
	cp -af $(SCRIPT_FILES) $(INSTALLDIR)

clean:

distclean:	clean
	rm -rf $(INSTALLDIR)/*

make_source.inform:
	@./genPfEnv.sh


#------------------------------------------------------------------------------

include $(BASEDIR)/build/Rules.common
