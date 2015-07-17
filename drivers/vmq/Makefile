KERNEL_DIR	:= /lib/modules/$(shell uname -r)/build
vmq-objs	:= balloc.o main.o
obj-m		:= vmq.o

ifeq	($(DEBUG), 1)
#EXTRA_CFLAGS	+=-DDEBUG
endif

all:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KERNEL_DIR) M=$(PWD) clean
