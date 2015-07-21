#
#------------------------------------------------------------------------------

ifndef	BASEDIR
BASEDIR			:=$(shell pwd)/../..
endif
include $(BASEDIR)/Rules.mk

#------------------------------------------------------------------------------

BUILD_COUNT		:= .build_count
BUILD_MODEL		:= .build_model
ifeq ($(wildcard $(BUILD_MODEL)),)
BUILD_RUN		:=
else
BUILD_RUN		:= $(shell cat $(BUILD_MODEL))
endif

#BINARY_DIR		= $(MODEL)
#BINARY_INSTALL	= $(shell pwd)/..

PREBUILD_LISTS	= cJSONFiles iperf 
PREBUILD_COUNT	:= $(shell echo $(PREBUILD_LISTS) | wc -w)
ifneq ($(wildcard $(BUILD_COUNT)),)
BUILDED_COUNT	:= $(shell cat $(BUILD_COUNT))
endif

ifneq ($(PREBUILD_COUNT), $(BUILDED_COUNT))
BUILD_RUN		:=
endif

export MODEL CHIPSET BUILD_HOST CROSS_COMPILE BUILD_MODEL BUILD_COUNT ARCH CORES PREBUILD_LISTS BASEDIR


ifneq ($(BUILD_RUN), $(MODEL))
all:
	@echo "# external/packages - Run"
	./packages.build
else
all:
	@echo
	@echo "# external/packages - Nothing to do"
	@echo
endif

#------------------------------------------------------------------------------

clean:


distclean:
	rm -rf $(BUILD_COUNT) $(BUILD_MODEL)


install:

