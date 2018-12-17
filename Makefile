CC = gcc
CFLAG = -std=c11 -Wall -c -I./

all: xheap.o

xheap.o:  xmalloc.c
	$(CC) $(CFLAG) $^ -o $@

clean:
	rm -f xheap.o
