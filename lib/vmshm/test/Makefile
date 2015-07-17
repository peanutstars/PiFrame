#
#------------------------------------------------------------------------------

TARGET			:=reader select-unselect writer

EXTRA_CFLAGS	:=-D_GNU_SOURCE -D__USE_GNU
EXTRA_CFLAGS	+=-DDEBUG
EXTRA_CFLAGS	+=-g
EXTRA_CFLAGS	+=-I../
EXTRA_CFLAGS	+=-I../../../drivers/vmshm
EXTRA_CFLAGS	+=-I../../../includes

EXTRA_LFLAGS	+=-L.. -lvmshm
EXTRA_LFLAGS	+=-lrt

#------------------------------------------------------------------------------
# DO NOT EDIT LINE BELOW
#------------------------------------------------------------------------------
include ../../../Rules.mk

all:	$(TARGET)

reader:	reader.o
	$(CC) -o $@ $^ $(LFLAGS)

select-unselect: select-unselect.o
	$(CC) -o $@ $^ $(LFLAGS)

writer: writer.o
	$(CC) -o $@ $^ $(LFLAGS)

clean:
	$(RM) -rf $(TARGET) *.o
