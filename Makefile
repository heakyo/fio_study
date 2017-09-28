all: fio

fio: fio.o libfio.o smalloc.o
	gcc $^ -o $@

libfio.o: libfio.c
	gcc $^ -c -o $@

smalloc.o: smalloc.c
	gcc $^ -c -o $@

.PHONY: clean
clean:
	rm -rf fio libfio.o
