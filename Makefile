CFLAGS = -g -O0 -Wall

all: fio

fio: fio.c libfio.o smalloc.o
	gcc $^ $(CFLAGS) -o $@

libfio.o: libfio.c
	gcc $^ -c $(CFLAGS) -o $@

smalloc.o: smalloc.c
	gcc $^ -c $(CFLAGS) -o $@

.PHONY: clean
clean:
	rm -rf fio *.o
