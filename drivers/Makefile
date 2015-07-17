
ifndef	BASEDIR
BASEDIR			:=$(shell pwd)/..
endif
include $(BASEDIR)/Rules.mk

VAR_K_CONFIG	= $(wildcard $(KERNEL_DIR)/.config)
VAR_K_BUILD		= $(wildcard $(KERNEL_DIR)/kernel/timeconst.h)

ifeq ($(VAR_K_CONFIG),)
    ifeq ($(ARCH), arm)
        VAR_BUILD_CONFIG	= build_config
        VAR_BUILD_KERNEL	= build_kernel
    endif
else
ifeq ($(VAR_K_BUILD),)
    ifeq ($(ARCH), arm)
        VAR_BUILD_KERNEL	= build_kernel
    endif
endif
endif


export BASEDIR DEBUG

INSTALLDIR		:=$(TARGETDIR)/system/driver/

MODS			:=vmq vmshm
KO_FILES		:=vmq/vmq.ko vmshm/vmshm.ko
SH_FILES		:=$(wildcard load.*)
obj-m			:=$(patsubst %,%/,$(MODS))

all: $(if $(VAR_BUILD_CONFIG), build_config) $(if $(VAR_BUILD_KERNEL), build_kernel)
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_DIR) M=$(shell pwd) modules BASEDIR=$(shell pwd)/..

build_config:
	$(MAKE) -C $(KERNEL_DIR) xcode6_nand_defconfig

build_kernel:
	$(MAKE) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KERNEL_DIR) -j$(CORES)

install:	all
	install -d $(INSTALLDIR)
	install $(KO_FILES) $(SH_FILES) $(INSTALLDIR)

clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(shell pwd) clean
	rm -rf Module.symvers modules.order

distclean:	clean
	rm -rf $(patsubst %,$(INSTALLDIR)/%.ko,$(MODS)) $(patsubst %, $(INSTALLDIR)/%, $(SH_FILES))

