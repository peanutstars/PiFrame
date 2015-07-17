KERNEL_DIR	:= /lib/modules/$(shell uname -r)/build
vmshm-objs	:= main.o
obj-m		:= vmshm.o

EXTRA_CFLAGS	+= -DEXPORT_SYMTAB
ifeq	($(DEBUG), 1)
#EXTRA_CFLAGS	+=-DDEBUG
endif

all:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean
