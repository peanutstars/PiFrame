
ifndef	BASEDIR
BASEDIR			:=$(shell pwd)/..
endif
include $(BASEDIR)/Rules.mk

#------------------------------------------------------------------------------

DEFAULT_DIRS	:=  CamPi


INSTALLDIR		:=$(TARGETDIR)/system

all:

install:	all
	install -d $(INSTALLDIR)
	install -d $(INSTALLDIR)/default
	cp -af $(DEFAULT_DIRS) $(INSTALLDIR)/default
	(for m in $(DEFAULT_DIRS); do rm -rf $(INSTALLDIR)/default/$$m/.svn; done;)

clean:

distclean:	clean
	rm -rf $(INSTALLDIR)/*


#------------------------------------------------------------------------------

include $(BASEDIR)/build/Rules.common
