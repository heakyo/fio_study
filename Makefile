CFLAGS = -c -g -O0 -Wall

all: fio

fio: fio.o libfio.o smalloc.o
	$(CC) -o $@ $^

%.o: %.c
	$(CC) $^ $(CFLAGS) -o $@

.PHONY: clean
clean:
	rm -rf fio *.o
