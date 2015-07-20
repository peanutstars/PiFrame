# ABOUT		: VMFramework main build makefile
#
#------------------------------------------------------------------------------

SUBDIRS	:= vmq vmshm pfconfig

#------------------------------------------------------------------------------
# DEFINE SUBDIR MACRO
#------------------------------------------------------------------------------
DISTCLEANSUBDIRS	:= $(patsubst %, %-distclean, $(SUBDIRS))
CLEANSUBDIRS		:= $(patsubst %, %-clean, $(SUBDIRS))
INSTALLSUBDIRS		:= $(patsubst %, %-install, $(SUBDIRS))

#------------------------------------------------------------------------------
# DEFINE SUBDIR RULES
#------------------------------------------------------------------------------
.PHONY: install install-subdirs clean clean-subdirs distclean distclean-subdirs $(SUBDIRS) $(INSTALLSUBDIRS) $(CLEANSUBDIRS) $(DISTCLEANSUBDIRS) tags

all:	$(SUBDIRS)

$(SUBDIRS):	
	$(MAKE) -C $@
	$(MAKE) -C $@ install

install:	install-subdirs

install-subdirs:	$(INSTALLSUBDIRS)

$(INSTALLSUBDIRS):
	$(MAKE) -C $(patsubst %-install,%,$@) install

clean:	clean-subdirs

clean-subdirs:	$(CLEANSUBDIRS)

$(CLEANSUBDIRS):
	$(MAKE) -C $(patsubst %-clean,%,$@) clean

distclean:	distclean-subdirs

distclean-subdirs:	$(DISTCLEANSUBDIRS)

$(DISTCLEANSUBDIRS):
	$(MAKE) -C $(patsubst %-distclean,%,$@) distclean

tags:
	@ctags -R
