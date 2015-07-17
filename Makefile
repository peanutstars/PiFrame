#
#------------------------------------------------------------------------------

ifndef	BASEDIR
BASEDIR			:=$(shell pwd)
endif
WHOAMI			:=$(shell whoami)
include $(BASEDIR)/Rules.mk

#------------------------------------------------------------------------------

#SUBDIRS			:= scripts lib
SUBDIRS			:= drivers util
						
#------------------------------------------------------------------------------

include $(BASEDIR)/build/Rules.common

remove-target:
	@rm -rf $(PF_OUTPUT_DIR)
