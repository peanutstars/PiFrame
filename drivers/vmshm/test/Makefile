CC	:= gcc
EXEC	:= writer reader select-unselect

all:	${EXEC}

clean:
	@rm -rf ${EXEC}

writer:	writer.c ../vmshm.h

reader:	reader.c ../vmshm.h

select-unselect:	select-unselect.c ../vmshm.h
