#
#------------------------------------------------------------------------------

ifndef	BASEDIR
BASEDIR			:=$(shell pwd)
endif
WHOAMI			:=$(shell whoami)
include $(BASEDIR)/Rules.mk

#------------------------------------------------------------------------------

#SUBDIRS			:= scripts lib
SUBDIRS			:= scripts drivers util
						
#------------------------------------------------------------------------------

include $(BASEDIR)/build/Rules.common

remove-target:
	@rm -rf $(PF_OUTPUT_DIR)
